#include "stdafx.h"
#include "Test.hpp"
#include "Application.hpp"
#include "Profiling.hpp"
#include "Scoring.hpp"
#include "Audio.hpp"
#include "Track.hpp"
#include "Camera.hpp"
#include "Background.hpp"
#include "GUI.hpp"

class Test_Impl : public Test
{
private:
	Ref<Canvas> m_canvas;
	GUIRenderer m_guiRenderer;

public:
	bool Init()
	{
		if(!m_guiRenderer.Init(g_gl))
			return false;

		m_canvas = Utility::MakeRef(new Canvas());

		return true;
	}
	~Test_Impl()
	{
	}

	virtual void OnKeyPressed(Key key)
	{
	}
	virtual void OnKeyReleased(Key key)
	{

	}
	virtual void Render(float deltaTime) override
	{
		Rect viewport(Vector2(), g_gameWindow->GetWindowSize());
		m_guiRenderer.Render(deltaTime, viewport, m_canvas.As<GUIElementBase>());
	}
	virtual void Tick(float deltaTime) override
	{
	}
};

Test* Test::Create()
{
	Test_Impl* impl = new Test_Impl();
	if(!impl->Init())
	{
		delete impl;
		return nullptr;
	}
	return impl;
}
