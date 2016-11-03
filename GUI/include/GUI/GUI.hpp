#pragma once
#include <GUI/GUITextInput.hpp>
#include <GUI/GUIRenderData.hpp>
#include <GUI/Focusable.hpp>

/*
	Class that manages the state of a GUI composition,
	handles routing of input to the appropriate GUI elements and 
	choosing which element can receive cursor focus
*/
class GUI
{
public:
	~GUI();

	// Binds the GUI to a given window, which will make it receive input events from this window
	bool Init(Graphics::Window* window = nullptr);

	// Updates the UI
	void Update(float deltaTime, Rect viewportSize = Rect::Empty);

	// Renders the UI
	void Render(GUIRenderer* renderer);

	// The root canvas which contains all other GUI elements
	Ref<class Canvas> GetRootCanvas();

	void SetWindow(Graphics::Window* window);
	Graphics::Window* GetWindow() const;

	// Mouse focus handle (hover)
	FocusHandle AcquireFocus(GUIElementBase* element, bool isKeyboardFocus = false);

	// Mouse down handle (clicked/held)
	FocusHandle AcquireMouseDown(GUIElementBase* element);

	// The element that has focus
	GUIElementBase* GetFocusedElement();

	// Set keyboard focus on an element
	void SetFocusedElement(GUIElementBase* element);

	// Set the last selected element, which is also the element that will get keyboard focus when navigating and no element is currently focused
	void SetLastSelectedElement(GUIElementBase* element);

	// Sends confirm selection (Enter, Confirm Button) to the focused gui element
	void ConfirmSelection();

	// Sends cancel selection (Esc, Back Button) to the focused gui element
	void CancelSelection();

	// Input focus handle (for text events and activated controls like sliders)
	FocusHandle AcquireInputFocus(GUIElementBase* element);

	// Get the text input for the current frame
	const struct GUITextInput& GetTextInput() const;

	const Vector2i& GetMousePos() const;
	const Vector2i& GetMouseDelta() const;

	// Check if a mouse button is currently pressed
	bool GetMouseButton(Graphics::MouseButton btn) const;
	// Check if mouse button was pressed since last frame
	bool GetMouseButtonPressed(Graphics::MouseButton btn) const;
	// Check if mouse button was released since last frame
	bool GetMouseButtonReleased(Graphics::MouseButton btn) const;

	// Vertical Mouse scroll
	//	Positive for scroll down
	//	Negative for scroll up
	int32 GetMouseScroll() const;

	// Navigate the focus Left/Right(layoutDirection=0) or Up/Down(layoutDirection=1) where dir = (-1,1) 
	void Navigate(int dir, int layoutDirection);
	// Navigate to the lastly focused element or the element set with SetLastSelectedElement
	void NavigateLast();

private:
	void m_OnTextInput(const WString& input);
	void m_OnTextComposition(const Graphics::TextComposition& input);
	void m_OnKeyRepeat(Graphics::Key key);
	void m_OnKeyPressed(Graphics::Key key);
	void m_OnKeyReleased(Graphics::Key key);
	void m_OnMousePressed(Graphics::MouseButton btn);
	void m_OnMouseReleased(Graphics::MouseButton btn);
	void m_OnMouseScroll(int32 scroll);

	// Releases the input focus on the current element, if any
	void m_ReleaseInputFocus();

	// Releases the focus on and element, if any
	void m_ReleaseFocus();

	// Call to reset text input state after render frame
	void m_ResetTextInput();

	Ref<Canvas> m_rootCanvas;

	GUITextInput m_textInput;
	Graphics::Window* m_window = nullptr;
	GUIElementBase* m_hoveredElement = nullptr;

	// The last focused element, to enable keyboard focus
	Ref<GUIElementBase> m_lastSelectedElement;

	bool m_mouseButtonState[3] = { 0 };
	bool m_mouseButtonStateLast[3] = { 0 };

	// Internal timer for sending to shaders that use it and maybe other uses
	float m_time;

	Vector2i m_mousePos;
	Vector2i m_mouseDelta;
	int32 m_mouseScrollDelta = 0;

	Rect m_viewport;

	FocusHandle m_mouseDownHandle;
	FocusHandle m_focusHandle;
	FocusHandle m_inputFocusHandle;
};