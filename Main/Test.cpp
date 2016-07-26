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
#include "HealthGauge.hpp"

class Test_Impl : public Test
{
private:
	Ref<Canvas> m_canvas;
	GUIRenderer m_guiRenderer;
	Ref<TextInputField> m_imeWindow;
	Ref<HealthGauge> m_healthGauge;

	WString m_currentText;

	Texture fillTexture;
	Material noise;

public:
	bool Init()
	{
		if(!m_guiRenderer.Init(g_gl, g_gameWindow))
			return false;

		m_canvas = Utility::MakeRef(new Canvas());
		m_imeWindow = Utility::MakeRef(new TextInputField());
		//Canvas::Slot* slot = m_canvas->Add(m_imeWindow.As<GUIElementBase>());
		//slot->autoSizeX = true;
		//slot->autoSizeY = true;
		//slot->anchor = Anchors::TopMiddle;
		//slot->anchor.top = 0.1f;
		//slot->anchor.bottom = 0.1f;
		//slot->alignment = Vector2(0.5f, 0.0f);
		//m_guiRenderer.SetInputFocus(m_imeWindow.GetData());
		m_healthGauge = Utility::MakeRef(new HealthGauge());
		Canvas::Slot* slot = m_canvas->Add(m_healthGauge.As<GUIElementBase>());
		slot->anchor = Anchor(0.95f, 0.5f);
		slot->autoSizeX = true;
		slot->autoSizeY = true;
		slot->alignment = Vector2(1.0f, 0.5f);
		
		return true;
	}
	~Test_Impl()
	{
	}
	virtual void OnKeyPressed(Key key) override
	{
		if(key == Key::ArrowUp)
			m_healthGauge->rate += 0.1f;
		else if(key == Key::ArrowDown)
			m_healthGauge->rate -= 0.1f;
		m_healthGauge->rate = Math::Clamp<float>(m_healthGauge->rate, 0, 1);
	}
	virtual void OnKeyReleased(Key key) override
	{
	}
	virtual void Render(float deltaTime) override
	{
		glClearColor(0, 0, 0, 0);
		glClear(GL_COLOR_BUFFER_BIT);
		Rect viewport(Vector2(), g_gameWindow->GetWindowSize());
		m_guiRenderer.Render(deltaTime, viewport, m_canvas.As<GUIElementBase>());

		//RenderQueue& rq = m_guiRenderer.Begin();
		//Transform t = Transform::Scale(Vector3(fillTexture->GetSize(), 1));
		//MaterialParameterSet params;
		//params.SetParameter("fillTexture", fillTexture);
		//params.SetParameter("rate", 0.5f);
		//rq.Draw(t, m_guiRenderer.guiQuad, noise, params);
		//m_guiRenderer.End();
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
