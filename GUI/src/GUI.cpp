#include "stdafx.h"
#include "GUI.hpp"
#include "GUIRenderer.hpp"
#include "GUIRenderData.hpp"
#include "GUIUpdateData.hpp"
#include "Focusable.hpp"
using namespace Graphics;

GUI::~GUI()
{
	SetWindow(nullptr);
}
bool GUI::Init(class Graphics::Window* window)
{
	m_time = 0.0f;

	// Initial window assignment
	SetWindow(window);

	// Create root canvas
	m_rootCanvas = Ref<Canvas>(new Canvas());
	m_rootCanvas->m_gui = this;

	return true;
}
void GUI::Render(GUIRenderer* renderer)
{
	// Render
	GUIRenderData renderData;
	renderData.debug = false; // Turn this on for debug visuals
	renderData.guiRenderer = renderer;
	renderData.rq = &renderer->Begin(m_viewport);
	m_rootCanvas->Render(renderData);

	renderer->End();
}
void GUI::Update(float deltaTime, Rect viewportSize /*= Rect::Empty*/)
{
	if(viewportSize == Rect::Empty)
		viewportSize = Rect(Vector2(), m_window->GetWindowSize());

	m_time += deltaTime;

	if(viewportSize != m_viewport)
	{
		m_viewport = viewportSize;

		InvalidationEvent event;
		event.invalidateArea = true;
		event.Propagate(*m_rootCanvas);
	}

	// Update mouse input
	Vector2i newMouse = m_window->GetMousePos();
	m_mouseDelta = newMouse - m_mousePos;
	m_mousePos = newMouse;

	// Send GUI events here
	if(m_mouseDelta != Vector2i::Zero)
	{
		MouseMovedEvent move;
		move.newPosition = m_mousePos;
		move.delta = m_mouseDelta;
		move.Propagate(*m_rootCanvas);
	}

	// Click events
	for(uint32 i = 0; i < 3; i++)
	{
		if(m_mouseButtonState[i] != m_mouseButtonStateLast[i])
		{
			MouseButtonEvent evt;
			evt.button = (MouseButton)i;
			evt.state = m_mouseButtonState[i];


			// Release keyboard focus on click
			if(evt.button == MouseButton::Left && evt.state)
			{
				// Release input focus when clicking somewhere else
				if(m_inputFocusHandle && (!m_focusHandle || m_focusHandle->element != m_inputFocusHandle->element))
				{
					m_ReleaseInputFocus();
				}
				if(m_focusHandle && m_focusHandle->keyboard)
				{
					// Release keyboard focus
					m_ReleaseFocus();
				}
			}

			if(evt.button == MouseButton::Left && !evt.state && m_mouseDownHandle)
			{
				auto focusElement = m_mouseDownHandle->element;
				evt.Propagate(*focusElement);
				m_mouseDownHandle.Release();
			}
			else if(m_focusHandle)
			{
				auto focusElement = m_focusHandle->element;
				evt.Propagate(*focusElement);
			}
		}
	}

	if(m_mouseDownHandle.GetRefCount() == 1)
	{
		m_focusHandle.Release();
	}

	// Reset focus handles
	if(m_focusHandle.GetRefCount() == 1)
	{
		m_ReleaseFocus();
	}

	if(m_inputFocusHandle.GetRefCount() == 1)
	{
		m_ReleaseInputFocus();
	}

	// Update animations on GUI
	m_rootCanvas->UpdateAnimations(deltaTime);

	// Update GUI
	GUIUpdateData updateData;
	updateData.debug = true;
	updateData.deltaTime = deltaTime;
	updateData.area = m_viewport;
	m_rootCanvas->Update(updateData);

	// Clear mouse scroll state
	m_mouseScrollDelta = 0;

	// Shift mouse button state
	memcpy(m_mouseButtonStateLast, m_mouseButtonState, sizeof(bool) * 3);

	// Clear text input after processing
	m_ResetTextInput();
}
Ref<Canvas> GUI::GetRootCanvas()
{
	return m_rootCanvas;
}
void GUI::SetWindow(Graphics::Window* window)
{
	// Unhook old window events
	if(m_window)
	{
		m_window->OnKeyRepeat.RemoveAll(this);
		m_window->OnTextInput.RemoveAll(this);
		m_window->OnTextComposition.RemoveAll(this);
		m_window->OnKeyPressed.RemoveAll(this);
		m_window->OnKeyReleased.RemoveAll(this);
		m_window->OnMousePressed.RemoveAll(this);
		m_window->OnMouseReleased.RemoveAll(this);
		m_window->OnMouseScroll.RemoveAll(this);
		m_window = nullptr;
	}

	m_window = window;

	// Hook new window events
	if(m_window)
	{
		m_window->OnKeyRepeat.Add(this, &GUI::m_OnKeyRepeat);
		m_window->OnTextInput.Add(this, &GUI::m_OnTextInput);
		m_window->OnTextComposition.Add(this, &GUI::m_OnTextComposition);
		m_window->OnKeyPressed.Add(this, &GUI::m_OnKeyPressed);
		m_window->OnKeyReleased.Add(this, &GUI::m_OnKeyReleased);
		m_window->OnMousePressed.Add(this, &GUI::m_OnMousePressed);
		m_window->OnMouseReleased.Add(this, &GUI::m_OnMouseReleased);
		m_window->OnMouseScroll.Add(this, &GUI::m_OnMouseScroll);
	}
}
Graphics::Window* GUI::GetWindow() const
{
	return m_window;
}
const GUITextInput& GUI::GetTextInput() const
{
	return m_textInput;
}
const Vector2i& GUI::GetMousePos() const
{
	return m_mousePos;
}
const Vector2i& GUI::GetMouseDelta() const
{
	return m_mouseDelta;
}
bool GUI::GetMouseButton(MouseButton btn) const
{
	return m_mouseButtonState[(size_t)btn];
}
bool GUI::GetMouseButtonPressed(MouseButton btn) const
{
	return m_mouseButtonState[(size_t)btn] && !m_mouseButtonStateLast[(size_t)btn];
}
bool GUI::GetMouseButtonReleased(MouseButton btn) const
{
	return !m_mouseButtonState[(size_t)btn] && m_mouseButtonStateLast[(size_t)btn];
}
int32 GUI::GetMouseScroll() const
{
	return m_mouseScrollDelta;
}
void GUI::SetFocusedElement(GUIElementBase* element)
{
	Focusable* focusable = dynamic_cast<Focusable*>(element);
	if(!focusable)
	{
		// Cant' focus this element
		return;
	}

	m_ReleaseFocus();

	focusable->Focus();
}
void GUI::SetLastSelectedElement(GUIElementBase* element)
{
	m_lastSelectedElement = element ? element->MakeShared() : GUIElement();
}
GUIElementBase* GUI::GetFocusedElement()
{
	if(m_focusHandle)
	{
		return m_focusHandle->element;
	}
	return nullptr;
}
void GUI::Navigate(int dir, int layoutDirection)
{
	GUIElementBase* selection = GetFocusedElement();
	if(!selection)
	{
		NavigateLast();
		return;
	}

	auto container = selection->GetParent();
	assert(container);
	GUIElementBase* next = selection;
	while(next != nullptr)
	{
		next = container->SelectNext(selection, selection, dir, layoutDirection);
		if(dynamic_cast<Focusable*>(next))
		{
			// Select new element
			SetFocusedElement(next);
			break;
		}
		if(next == selection)
			break; // Abort selection if next element doesn't change anymore
	}
}
void GUI::NavigateLast()
{
	if(!m_lastSelectedElement)
		return;

	// Make sure this element is still in the hierarchy
	if(m_lastSelectedElement.GetRefCount() == 1)
		return;
	if(m_lastSelectedElement->m_gui != this)
		return;

	SetFocusedElement(m_lastSelectedElement.GetData());
}
FocusHandle GUI::AcquireFocus(GUIElementBase* element, bool isKeyboardFocus)
{
	Focusable* focusable = dynamic_cast<Focusable*>(element);
	if(!focusable)
	{
		// Cant' focus this element
		return FocusHandle();
	}

	if(!element)
		return FocusHandle();

	// Lock handle if mouse down is in progress
	if(m_mouseDownHandle && element != m_mouseDownHandle->element)
		return FocusHandle();

	// Release previous
	if(m_focusHandle)
	{
		bool allowFocus = false;

		// Allow focus break when the current selection is a keyboard selection and the new selection is also a keyboard selection
		if(m_focusHandle->keyboard && isKeyboardFocus)
		{
			allowFocus = true;
		}
		else
		{
			// Check if new element is a child of the current focus element, in that case allow selection
			GUIElementBase* findParent = element;
			while(findParent)
			{
				if(findParent == m_focusHandle->element)
				{
					allowFocus = true;
					break;
				}
				findParent = findParent->GetParent();
			}
		}

		if(!allowFocus)
			return FocusHandle(); // Don't switch focus

		m_ReleaseFocus();
	}

	// Store last selected
	m_lastSelectedElement = element->MakeShared();

	// New handle
	m_focusHandle = Utility::MakeRef(new  FocusHandleData{isKeyboardFocus, element, focusable});
	focusable->OnFocus();
	return m_focusHandle;
}
FocusHandle GUI::AcquireMouseDown(GUIElementBase* element)
{
	Focusable* focusable = dynamic_cast<Focusable*>(element);
	if(!focusable)
	{
		// Cant' focus this element
		return FocusHandle();
	}

	if(!element)
		return FocusHandle();

	// Can't aquire twice
	if(m_mouseDownHandle)
	{
		return FocusHandle();
	}

	// New handle
	m_mouseDownHandle = Utility::MakeRef(new  FocusHandleData{false, element, focusable});
	return m_mouseDownHandle;
}
void GUI::ConfirmSelection()
{
	if(m_focusHandle)
	{
		if(m_focusHandle->focusable->CanEngage() && !m_inputFocusHandle)
		{
			// Engage element
			m_focusHandle->focusable->Engage();
			return;
		}
		return m_focusHandle->focusable->OnConfirm();
	}
}
void GUI::CancelSelection()
{
	if(m_focusHandle)
	{
		if(m_inputFocusHandle)
		{
			// Disengage element
			m_ReleaseInputFocus();
			return;
		}
		return m_focusHandle->focusable->OnCancel();
	}
}
FocusHandle GUI::AcquireInputFocus(GUIElementBase* element)
{
	Focusable* focusable = dynamic_cast<Focusable*>(element);
	if(!focusable)
	{
		// Cant' focus this element
		return FocusHandle();
	}

	if(!element)
		return FocusHandle();

	// Release previous
	if(m_inputFocusHandle)
	{
		m_ReleaseInputFocus();
	}

	// Start allowing IME text input
	m_window->StartTextInput();

	// New handle
	m_inputFocusHandle = Utility::MakeRef(new FocusHandleData{true, element, focusable});
	focusable->OnFocus();
	return m_inputFocusHandle;
}
void GUI::m_OnTextInput(const WString& input)
{
	m_textInput.input += input;
}
void GUI::m_OnTextComposition(const TextComposition& input)
{
	m_textInput.composition = input.composition;
}
void GUI::m_OnKeyRepeat(Key key)
{
	if(key == Key::Backspace)
	{
		if(m_textInput.input.empty())
			m_textInput.backspaceCount++; // Send backspace
		else
		{
			auto it = m_textInput.input.end(); // Modify input string instead
			--it;
			m_textInput.input.erase(it);
		}
	}

	if(m_inputFocusHandle)
	{
		KeyEvent event;
		event.key = key;
		event.keyState = true;
		event.repeated = true;
		m_inputFocusHandle->focusable->OnKeyEvent(event);
	}
	else
	{
		if(key == Key::ArrowUp)
		{
			Navigate(-1, 1);
		}
		else if(key == Key::ArrowDown)
		{
			Navigate(1, 1);
		}
		else if(key == Key::ArrowLeft)
		{
			Navigate(-1, 0);
		}
		else if(key == Key::ArrowRight)
		{
			Navigate(1, 0);
		}
	}
}
void GUI::m_OnKeyPressed(Key key)
{
	if(m_inputFocusHandle)
	{
		KeyEvent event;
		event.key = key;
		event.keyState = true;
		event.repeated = false;
		m_inputFocusHandle->focusable->OnKeyEvent(event);
	}
	else
	{
		if(key == Key::V)
		{
			if(m_window->GetModifierKeys() == ModifierKeys::Ctrl)
			{
				if(m_window->GetTextComposition().composition.empty())
				{
					// Paste clipboard text into input buffer
					m_textInput.input += m_window->GetClipboard();
				}
			}
		}
		else if(key == Key::Return)
		{
			ConfirmSelection();
		}
		else if(key == Key::Escape)
		{
			CancelSelection();
		}
	}
}
void GUI::m_OnKeyReleased(Graphics::Key key)
{
	if(m_inputFocusHandle)
	{
		KeyEvent event;
		event.key = key;
		event.keyState = false;
		event.repeated = false;
		m_inputFocusHandle->focusable->OnKeyEvent(event);
	}
}
void GUI::m_OnMousePressed(MouseButton btn)
{
	m_mouseButtonState[(size_t)btn] = true;
}
void GUI::m_OnMouseReleased(MouseButton btn)
{
	m_mouseButtonState[(size_t)btn] = false;
}
void GUI::m_OnMouseScroll(int32 scroll)
{
	m_mouseScrollDelta += scroll;
}
void GUI::m_ReleaseInputFocus()
{
	// Release previous input focus
	if(m_inputFocusHandle)
	{
		// Stop accepting IME input
		m_window->StopTextInput();
		m_inputFocusHandle->focusable->OnInputFocusLost();
		m_inputFocusHandle.Destroy();
	}
}
void GUI::m_ReleaseFocus()
{
	// Release previous focus
	if(m_focusHandle)
	{
		m_focusHandle->focusable->OnFocusLost();
		m_focusHandle.Destroy();
	}
}
void GUI::m_ResetTextInput()
{
	m_textInput.backspaceCount = 0;
	m_textInput.input.clear();
}

WString GUITextInput::Apply(const WString& in) const
{
	WString res = in + input;
	auto it = res.end();
	for(uint32 i = 0; i < backspaceCount; i++)
	{
		if(res.empty())
			break;
		--it;
		it = res.erase(it);
	}
	return res;
}
bool GUITextInput::HasChanges() const
{
	return input.size() != backspaceCount;
}
