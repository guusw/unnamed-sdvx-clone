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
	Ref<TextInputField> m_imeWindow;

	WString m_currentText;

public:
	bool Init()
	{
		if(!m_guiRenderer.Init(g_gl, g_gameWindow))
			return false;

		m_canvas = Utility::MakeRef(new Canvas());
		m_imeWindow = Utility::MakeRef(new TextInputField());
		Canvas::Slot* slot = m_canvas->Add(m_imeWindow.As<GUIElementBase>());
		slot->autoSizeX = true;
		slot->autoSizeY = true;
		slot->anchor = Anchors::TopMiddle;
		slot->anchor.top = 0.1f;
		slot->anchor.bottom = 0.1f;
		slot->alignment = Vector2(0.5f, 0.0f);

		m_guiRenderer.SetInputFocus(m_imeWindow.GetData());

		return true;
	}
	~Test_Impl()
	{
	}
	void OnKeyRepeat(Key key)
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
