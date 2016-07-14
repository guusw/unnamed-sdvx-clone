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

class IMEWindow : public LayoutBox
{
public:
	IMEWindow()
	{
		layoutDirection = LayoutBox::Horizontal;

		label = new Label();
		Slot* slot = Add(label->MakeShared());
		slot->fill = false;
		label->SetText(L"");
		label->SetFontSize(32);

		add = new Label();
		add->color = Color::Blue;
		slot = Add(add->MakeShared());
		slot->fill = false;
		add->SetText(L"");
		add->SetFontSize(32);
	}
	virtual void Render(GUIRenderData rd) override
	{
		LayoutBox::Render(rd);
	}
	virtual void SetText(const WString& text)
	{
		label->SetText(text);
	}
	virtual void SetComposition(const TextComposition& comp)
	{
		add->SetText(comp.composition);
	}

	Label* label;
	Label* add;
};

class Test_Impl : public Test
{
private:
	Ref<Canvas> m_canvas;
	GUIRenderer m_guiRenderer;
	Ref<IMEWindow> m_imeWindow;

	WString m_currentText;

public:
	bool Init()
	{
		if(!m_guiRenderer.Init(g_gl))
			return false;

		m_canvas = Utility::MakeRef(new Canvas());
		m_imeWindow = Utility::MakeRef(new IMEWindow());
		Canvas::Slot* slot = m_canvas->Add(m_imeWindow.As<GUIElementBase>());
		slot->autoSizeX = true;
		slot->autoSizeY = true;
		slot->anchor = Anchors::TopMiddle;
		slot->anchor.top = 0.1f;
		slot->anchor.bottom = 0.1f;
		slot->alignment = Vector2(0.5f, 0.0f);

		g_gameWindow->StartTextInput();
		g_gameWindow->OnTextInput.Add(this, &Test_Impl::OnTextInput);
		g_gameWindow->OnTextComposition.Add(this, &Test_Impl::OnTextComposition);
		g_gameWindow->OnKeyRepeat.Add(this, &Test_Impl::OnKeyRepeat);

		return true;
	}
	~Test_Impl()
	{
	}

	void OnTextInput(const WString& str)
	{
		m_currentText += str;
		m_imeWindow->SetText(m_currentText);
	}
	void OnTextComposition(const TextComposition& comp)
	{
		m_imeWindow->SetComposition(comp);
	}
	virtual void OnKeyPressed(Key key)
	{
	}
	void OnKeyRepeat(Key key)
	{
		if(key == Key::Backspace)
		{
			if(m_currentText.size() > 0)
			{
				auto it = m_currentText.end();
				--it;
				m_currentText.erase(it);
				m_imeWindow->SetText(m_currentText);
			}
		}
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
