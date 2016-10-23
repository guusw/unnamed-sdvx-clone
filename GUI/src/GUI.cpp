#include "stdafx.h"
#include "GUI.hpp"
#include "GUIRenderer.hpp"
#include "GUIRenderData.hpp"
#include "GUIUpdateData.hpp"
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
	renderData.debug = true; // Turn this on for debug visuals
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

			if(evt.button == MouseButton::Left && !evt.state && m_mouseDownHandle)
			{
				auto focusElement = m_mouseDownHandle.GetData<GUIElementBase>();
				evt.Propagate(*focusElement);
				m_mouseDownHandle.Release();
			}
			else if(m_mouseFocusHandle)
			{
				auto focusElement = m_mouseFocusHandle.GetData<GUIElementBase>();
				evt.Propagate(*focusElement);
			}
		}
	}

	if(m_mouseDownHandle.GetRefCount() == 1)
	{
		m_mouseFocusHandle.Release();
	}

	// Reset focus handles
	if(m_mouseFocusHandle.GetRefCount() == 1)
	{
		auto focusElement = m_mouseFocusHandle.GetData<GUIElementBase>();

		focusElement->OnFocusLost();
		m_mouseFocusHandle.Release();
	}

	if(m_inputFocusHandle.GetRefCount() == 1)
	{
		// Stop accepting IME input
		m_window->StopTextInput();
		m_inputFocusHandle.Release();
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
Handle GUI::AcquireMouseFocus(GUIElementBase* element)
{
	if(!element)
		return Handle();

	// Lock handle if mouse down is in progress
	if(m_mouseDownHandle && element != m_mouseDownHandle.GetData<GUIElementBase>())
		return Handle();

	// Release previous
	if(m_mouseFocusHandle)
	{
		GUIElementBase* existingHandle = m_mouseFocusHandle.GetData<GUIElementBase>();

		// Check if new element is a child of the current focus element, in that case allow selection
		GUIElementBase* findParent = element;
		bool allowFocus = false;
		while(findParent)
		{
			if(findParent == existingHandle)
			{
				allowFocus = true;
				break;
			}
			findParent = findParent->GetParent();
		}

		if(!allowFocus)
			return Handle(); // Don't switch focus

		m_mouseFocusHandle.GetData<GUIElementBase>()->OnFocusLost();
		m_mouseFocusHandle.ForceRelease();
	}

	// New handle
	m_mouseFocusHandle = Handle::Create(element);
	element->OnFocus();
	return m_mouseFocusHandle;
}
Handle GUI::AcquireMouseDown(GUIElementBase* element)
{
	if(!element)
		return Handle();

	// Can't aquire twice
	if(m_mouseDownHandle)
	{
		return Handle();
	}

	// New handle
	m_mouseDownHandle = Handle::Create(element);
	return m_mouseDownHandle;
}
GUIElementBase* GUI::GetFocusedElement()
{
	if(m_mouseFocusHandle)
	{
		return m_mouseFocusHandle.GetData<GUIElementBase>();
	}
	return nullptr;
}
Handle GUI::AcquireInputFocus(GUIElementBase* element)
{
	if(!element)
		return Handle();

	// Release previous
	if(m_inputFocusHandle)
	{
		m_inputFocusHandle.ForceRelease();
		m_window->StopTextInput(); // Reset IME state
	}

	// Start allowing IME text input
	m_window->StartTextInput();

	// New handle
	m_inputFocusHandle = Handle::Create(element);
	element->OnFocus();
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
}
void GUI::m_OnKeyPressed(Key key)
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
