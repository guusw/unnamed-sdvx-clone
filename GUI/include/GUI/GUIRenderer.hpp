#pragma once

struct GUITextInput
{
public:
	// Apply text input to a string
	WString Apply(const WString& in) const;
	bool HasChanges() const;

	class GUIElementBase* elementFocus = nullptr;
	// Input string that was entered since last frame
	WString input;
	// Number of backspaces last frame
	uint32 backspaceCount = 0;
	// IME Composition string
	WString composition;
};

class GUIRenderer
{
public:
	~GUIRenderer();

	// Loads gui shaders/textures/etc.
	bool Init(class OpenGL* gl, Graphics::Window* window = nullptr);

	// Updates and renders the GUI
	void Render(float deltaTime, Rect viewportSize, Ref<class GUIElementBase> rootElement);

	// Use to manually start rendering GUI elements
	RenderQueue& Begin();
	// Use to manually stop rendering GUI elements
	void End();

	void SetWindow(Graphics::Window* window);
	Graphics::Window* GetWindow() const;

	void PushScissorRect(const Rect& scissor);
	void PopScissorRect();
	Rect GetScissorRect() const;

	// The element that the mouse hovers over
	GUIElementBase* GetHoveredElement();

	// Set current input focus, this element receives keyboard text events
	void SetInputFocus(class GUIElementBase* element);
	// Get currently focused element
	class GUIElementBase* GetInputFocus() const;

	// Get the text input for the current frame
	const GUITextInput& GetTextInput() const;

	Vector2i RenderText(const String& str, const Vector2& position, const Color& color = Color(1.0f), uint32 fontSize = 16);
	Vector2i RenderText(const WString& str, const Vector2& position, const Color& color = Color(1.0f), uint32 fontSize = 16);
	void RenderText(Text& text, const Vector2& position, const Color& color = Color(1.0f));
	void RenderRect(const Rect& rect, const Color& color = Color(1.0f), Texture texture = Texture());
	void RenderButton(const Rect& rect, Texture texture, Margini border, const Color& color = Color::White);

	/*** New API ***/
	// Renders text
	void RenderText(const Transform2D& transform, Text& text, const Color& color = Color::White);
	void RenderText(const Transform2D& transform, const Rect& scissorRectangle, Text& text, const Color& color = Color::White);
	// Renders a rectangle with an optional texture and color
	void RenderRect(const Transform2D& transform, const Color& color = Color::White, Texture texture = Texture());
	void RenderRect(const Transform2D& transform, const Rect& scissorRectangle, const Color& color = Color::White, Texture texture = Texture());
	// Renders a wireframe box
	void RenderWireBox(const Transform2D& boxTransform, const Color& color = Color::White, float lineWidth = 1.0f);
	// Renders a single point
	void RenderPoint(const Transform2D& transform, const Color& color = Color::White, float pointSize = 1.0f);

	const Vector2i& GetMousePos() const;
	const Vector2i& GetMouseDelta() const;

	// Check if a mouse button is currently pressed
	bool GetMouseButton(MouseButton btn) const;
	// Check if mouse button was pressed since last frame
	bool GetMouseButtonPressed(MouseButton btn) const;
	// Check if mouse button was released since last frame
	bool GetMouseButtonReleased(MouseButton btn) const;

	// Vertical Mouse scroll
	//	Positive for scroll down
	//	Negative for scroll up
	int32 GetMouseScroll() const;

	// Default used font
	Graphics::Font font;
	// Material used for rendering fonts
	Material fontMaterial;

	// Textured/Color-Only materials
	Material textureMaterial;
	Material colorMaterial; 
	// Button material
	Material buttonMaterial;

	// Quad material for drawing gui elements
	Mesh guiQuad;
	// Line box
	Mesh lineBox;
	// Used for buttons since they use a geometry shader
	Mesh pointMesh;

private:
	void m_OnTextInput(const WString& input);
	void m_OnTextComposition(const TextComposition& input);
	void m_OnKeyRepeat(Key key);
	void m_OnKeyPressed(Key key);
	void m_OnMousePressed(MouseButton btn);
	void m_OnMouseReleased(MouseButton btn);
	void m_OnMouseScroll(int32 scroll);
	// Call to reset text input state after render frame
	void m_ResetTextInput();

	GUITextInput m_textInput;
	OpenGL* m_gl;
	Graphics::Window* m_window = nullptr;
	RenderQueue* m_renderQueue = nullptr;
	GUIElementBase* m_hoveredElement = nullptr;

	bool m_mouseButtonState[3] = { 0 };
	bool m_mouseButtonStateLast[3] = { 0 };

	// Internal timer for sending to shaders that use it and maybe other uses
	float m_time;

	Vector2i m_mousePos;
	Vector2i m_mouseDelta;
	int32 m_mouseScrollDelta = 0;

	Rect m_viewportSize;

	// Size.x < 0 means disabled
	Rect m_scissorRect;
	// Stack of scissor rectangles
	Vector<Rect> m_scissorRectangles;
};

