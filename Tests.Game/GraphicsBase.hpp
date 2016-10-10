#pragma once
#include <Graphics/Graphics.hpp>
using namespace Graphics;
#include <GUI/GUI.hpp>

/*
	Minimal graphics setup with just a window and an OpenGL context
*/
class GraphicsTestBase : public Unique
{
public:
	~GraphicsTestBase();
	virtual void Init() {};
	virtual void Render(float deltaTime) = 0;
	bool Run();

protected:
	Graphics::Window* m_window = nullptr;
	Graphics::OpenGL* m_gl = nullptr;
};

/*
	Simple GUI setup with Common Styles, a canvas and a GUIRenderer
*/
class GUITestBase : public GraphicsTestBase
{
public:
	virtual void Init() override
	{
		style = Ref<CommonGUIStyle>(new CommonGUIStyle(m_gl));
		canvas = Ref<Canvas>(new Canvas());
		guiRenderer = Ref<GUIRenderer>(new GUIRenderer());
		guiRenderer->Init(m_gl, m_window);
	}

	Ref<CommonGUIStyle> style;
	Ref<GUIRenderer> guiRenderer;
	Ref<Canvas> canvas;
};