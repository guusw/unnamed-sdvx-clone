#pragma once
#include <GUI/GUIRenderData.hpp>

class GUIRenderer
{
public:
	~GUIRenderer();

	// Loads gui shaders/textures/etc.
	bool Init(class Graphics::OpenGL* gl);

	// Use to start rendering GUI
	// fill the render queue with draw commands using any of the render functions on the GUIRenderer
	Graphics::RenderQueue& Begin(const Rect& viewport);
	// Use to stop rendering GUI
	void End();

	void PushScissorRect(const Rect& scissor);
	void PopScissorRect();
	Rect GetScissorRect() const;

	// Renders text
	void RenderText(const Transform2D& transform, Graphics::Text text, const Color& color = Color::White);
	void RenderText(const Transform2D& transform, const Rect& scissorRectangle, Graphics::Text text, const Color& color = Color::White);
	// Renders a rectangle with an optional texture and color
	void RenderRect(const Transform2D& transform, const Color& color = Color::White, Graphics::Texture texture = Graphics::Texture());
	void RenderRect(const Transform2D& transform, const Rect& scissorRectangle, const Color& color = Color::White, Graphics::Texture texture = Graphics::Texture());
	// Renders a wireframe box
	void RenderWireBox(const Transform2D& boxTransform, const Color& color = Color::White, float lineWidth = 1.0f);
	// Renders a single point
	void RenderPoint(const Transform2D& transform, const Color& color = Color::White, float pointSize = 1.0f);
	// Renders a line
	void RenderLine(const Transform2D& transform, const Color& color = Color::White, float pointSize = 1.0f);
	void RenderLine(const Vector2& a, const Vector2& b, const Color& color = Color::White, float pointSize = 1.0f);

	// Default used font
	Graphics::Font font;
	// Material used for rendering fonts
	Graphics::Material fontMaterial;

	// Textured/Color-Only materials
	Graphics::Material textureMaterial;
	Graphics::Material colorMaterial;
	// Button material
	Graphics::Material buttonMaterial;

	// Quad material for drawing gui elements
	Graphics::Mesh guiQuad;
	// Line box
	Graphics::Mesh lineBox;
	// Used for buttons since they use a geometry shader
	Graphics::Mesh pointMesh;
	// Line segment
	Graphics::Mesh lineMesh;

private:
	Graphics::OpenGL* m_gl;
	Graphics::RenderQueue* m_renderQueue = nullptr;

	// Internal timer for sending to shaders that use it and maybe other uses
	float m_time;

	Rect m_viewport;

	// Size.x < 0 means disabled
	Rect m_scissorRect;
	// Stack of scissor rectangles
	Vector<Rect> m_scissorRectangles;
};

