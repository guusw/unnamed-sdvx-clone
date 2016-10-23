#pragma once
#include <Graphics/Graphics.hpp>
using namespace Graphics;
#include <GUI/GUI.hpp>
#include <GUI/GUIRenderer.hpp>
#include <GUI/CommonGUIStyle.hpp>
#include <GUI/Canvas.hpp>

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
		gui = Ref<GUI>(new GUI());
		guiRenderer = Ref<GUIRenderer>(new GUIRenderer());
		gui->Init(m_window);
		guiRenderer->Init(m_gl);
		canvas = gui->GetRootCanvas();
	}
	virtual void Render(float deltaTime) override;

	Ref<CommonGUIStyle> style;
	Ref<GUIRenderer> guiRenderer;
	Ref<GUI> gui;
	Ref<Canvas> canvas;
};