#include "stdafx.h"
#include "GraphicsBase.hpp"
#include <GUI/GUI.hpp>

class GUITestBase : public GraphicsTestBase
{
public:
	virtual void Init() override
	{
		style = Ref<CommonGUIStyle>(new CommonGUIStyle(m_gl));
		canvas = Ref<Canvas>(new Canvas());
		guiRenderer = Ref<GUIRenderer>(new GUIRenderer());
		guiRenderer->Init(m_gl, m_window);

		Ref<Panel> test = Ref<Panel>(new Panel());
		test->color = Color::Cyan;
		auto slot = canvas->Add(test.As<GUIElementBase>());
		slot->anchor = Anchor(0.5f);
		slot->alignment = Vector2(0.5f);
		slot->offset.size = Vector2(100.0f);
		slot->offset.pos = Vector2(-50.0f);

	}
	virtual void Render(float deltaTime) override
	{
		Rect viewport;
		viewport.size = m_window->GetWindowSize();
		glViewport(0, 0, viewport.size.x, viewport.size.y);
		glClearColor(0, 0, 0, 0);
		glClear(GL_COLOR_BUFFER_BIT);
		guiRenderer->Render(deltaTime, viewport, canvas.As<GUIElementBase>());

		m_gl->SwapBuffers();
	}

	Ref<CommonGUIStyle> style;
	Ref<GUIRenderer> guiRenderer;
	Ref<Canvas> canvas;
};

Test("GUI")
{
	GUITestBase test;
	test.Run();
}