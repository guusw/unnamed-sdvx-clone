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
	bool Init(class OpenGL* gl, Window* window = nullptr);
	void Render(float deltaTime, Rect viewportSize, Ref<class GUIElementBase> rootElement);

	// Use to manually start rendering GUI elements
	RenderQueue& Begin();
	// Use to manually stop rendering GUI elements
	void End();

	void SetWindow(Window* window);
	Window* GetWindow() const;

	void PushScissorRect(const Rect& scissor);
	void PopScissorRect();

	// Set current input focus, this element receives keyboard text events
	void SetInputFocus(class GUIElementBase* element);
	// Get currently focused element
	class GUIElementBase* GetInputFocus() const;

	// Get the text input for the current frame
	const GUITextInput& GetTextInput() const;

	// Renders text, returns the size of the rendered text
	Vector2i GetTextSize(const String& str, uint32 fontSize = 16);
	Vector2i GetTextSize(const WString& str, uint32 fontSize = 16);
	Vector2i RenderText(RenderQueue& rq, const String& str, const Vector2& position, const Color& color = Color(1.0f), uint32 fontSize = 16);
	Vector2i RenderText(RenderQueue& rq, const WString& str, const Vector2& position, const Color& color = Color(1.0f), uint32 fontSize = 16);
	void RenderText(RenderQueue& rq, Text& text, const Vector2& position, const Color& color = Color(1.0f));
	// Draws a rectangle, either with a texture or just a color
	void RenderRect(RenderQueue& rq, const Rect& rect, const Color& color = Color(1.0f), Texture texture = Texture());

	// Default used font
	Font font;
	// Material used for rendering fonts
	Material fontMaterial;

	// Textured/Color-Only materials
	Material textureMaterial;
	Material colorMaterial; 

	// Quad material for drawing gui elements
	Mesh guiQuad;

private:
	void m_OnTextInput(const WString& input);
	void m_OnTextComposition(const TextComposition& input);
	void m_OnKeyRepeat(Key key);
	// Call to reset text input state after render frame
	void m_ResetTextInput();

	GUITextInput m_textInput;
	OpenGL* m_gl;
	Window* m_window = nullptr;
	RenderQueue* m_renderQueue = nullptr;

	// Size.x < 0 means disabled
	Rect m_scissorRect;
	// Stack of scissor rectangles
	Vector<Rect> m_scissorRectangles;
};

