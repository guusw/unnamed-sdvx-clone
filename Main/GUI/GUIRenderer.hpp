#pragma once

class GUIRenderer
{
public:
	// Loads gui shaders/textures/etc.
	bool Init(class OpenGL* gl);
	void Render(float deltaTime, Rect viewportSize, Ref<class GUIElementBase> rootElement);

	void SetScissorRect(const Rect& scissor);
	void ClearScissorRect();

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
	OpenGL* m_gl;
	RenderQueue* m_renderQueue = nullptr;

	// Size.x < 0 means disabled
	Rect m_scissorRect;
};

