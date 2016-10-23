#pragma once
#include <GUI/GUITextInput.hpp>
#include <GUI/GUIRenderData.hpp>
#include <Shared/Handle.hpp>

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
	Handle AcquireMouseFocus(class GUIElementBase* element);

	// Mouse down handle (clicked/held)
	Handle AcquireMouseDown(class GUIElementBase* element);

	// The element that has mouse focus
	GUIElementBase* GetFocusedElement();

	// Input focus handle (for text events)
	Handle AcquireInputFocus(class GUIElementBase* element);

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

private:
	void m_OnTextInput(const WString& input);
	void m_OnTextComposition(const Graphics::TextComposition& input);
	void m_OnKeyRepeat(Graphics::Key key);
	void m_OnKeyPressed(Graphics::Key key);
	void m_OnMousePressed(Graphics::MouseButton btn);
	void m_OnMouseReleased(Graphics::MouseButton btn);
	void m_OnMouseScroll(int32 scroll);
	// Call to reset text input state after render frame
	void m_ResetTextInput();

	Ref<Canvas> m_rootCanvas;

	GUITextInput m_textInput;
	Graphics::Window* m_window = nullptr;
	GUIElementBase* m_hoveredElement = nullptr;

	bool m_mouseButtonState[3] = { 0 };
	bool m_mouseButtonStateLast[3] = { 0 };

	// Internal timer for sending to shaders that use it and maybe other uses
	float m_time;

	Vector2i m_mousePos;
	Vector2i m_mouseDelta;
	int32 m_mouseScrollDelta = 0;

	Rect m_viewport;

	Handle m_mouseFocusHandle;
	Handle m_mouseDownHandle;
	Handle m_inputFocusHandle;
};