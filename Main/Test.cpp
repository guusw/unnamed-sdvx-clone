#include "stdafx.h"
#include "Test.hpp"
#include "Application.hpp"
#include <Shared/Profiling.hpp>
#include "Scoring.hpp"
#include <AUdio/Audio.hpp>
#include "Track.hpp"
#include "Camera.hpp"
#include "Background.hpp"
#include "GUI.hpp"
#include "GUI/CommonGUIStyle.hpp"
#include "GUI/Button.hpp"
#include "GUI/Slider.hpp"
#include "GUI/ScrollBox.hpp"
#include "GUI/SettingsBar.hpp"
#include "GUI/Spinner.hpp"
#include "HealthGauge.hpp"
#include "Shared/Jobs.hpp"
#include "ScoreScreen.hpp"
#include "Shared/Enum.hpp"

class Test_Impl : public Test
{
private:
	Ref<CommonGUIStyle> m_guiStyle;
	Ref<SettingsBar> m_settings;

	WString m_currentText;
	float a = 0.1f; // 0 - 1
	float b = 2.0f; // 0 - 10
	float c = 1.0f; // 0 - 5
	float d = 0.0f; // -2 - 2

	Ref<Gamepad> m_gamepad;

public:
	static void StaticFunc(int32 arg)
	{
	}
	static int32 StaticFunc1(int32 arg)
	{
		return arg * 2;
	}
	static int32 StaticFunc2(int32 arg)
	{
		return arg * 2;
	}
	bool Init()
	{
		m_guiStyle = CommonGUIStyle::Get();

		m_gamepad = g_gameWindow->OpenGamepad(0);

		{
			ScrollBox* box0 = new ScrollBox(m_guiStyle);
			Canvas::Slot* slot = g_rootCanvas->Add(box0->MakeShared());
			slot->anchor = Anchor(0.0f, 0.0f);
			slot->offset = Rect(Vector2(10.0f, 10.0f), Vector2(500, 500.0f));
			slot->autoSizeX = false;
			slot->autoSizeY = false;

			{
				LayoutBox* box = new LayoutBox();
				box->layoutDirection = LayoutBox::Vertical;
				box0->SetContent(box->MakeShared());

				LayoutBox::Slot* btnSlot;
				Button* btn0 = new Button(m_guiStyle);
				btn0->SetText(L"TestButton0");
				btn0->SetFontSize(32);
				btnSlot = box->Add(btn0->MakeShared());
				btnSlot->padding = Margin(2);

				Button* btn1 = new Button(m_guiStyle);
				btn1->SetText(L"This is a button with slightly\nlarger text");
				btn1->SetFontSize(32);
				btnSlot = box->Add(btn1->MakeShared());
				btnSlot->padding = Margin(2);

				TextInputField* fld = new TextInputField(m_guiStyle);
				fld->SetText(L"textinput");
				fld->SetFontSize(32);
				btnSlot = box->Add(fld->MakeShared());
				btnSlot->fillX = true;
				btnSlot->padding = Margin(2);

				Slider* sld = new Slider(m_guiStyle);
				btnSlot = box->Add(sld->MakeShared());
				btnSlot->fillX = true;
				btnSlot->padding = Margin(2);

				sld = new Slider(m_guiStyle);
				btnSlot = box->Add(sld->MakeShared());
				btnSlot->fillX = true;
				btnSlot->padding = Margin(2);

				sld = new Slider(m_guiStyle);
				btnSlot = box->Add(sld->MakeShared());
				btnSlot->fillX = true;
				btnSlot->padding = Margin(2);

				// Duplicate items
				btn0 = new Button(m_guiStyle);
				btn0->SetText(L"TestButton0");
				btn0->SetFontSize(32);
				btnSlot = box->Add(btn0->MakeShared());
				btnSlot->padding = Margin(2);

				btn1 = new Button(m_guiStyle);
				btn1->SetText(L"This is a button with slightly\nlarger text");
				btn1->SetFontSize(32);
				btnSlot = box->Add(btn1->MakeShared());
				btnSlot->padding = Margin(2);

				fld = new TextInputField(m_guiStyle);
				fld->SetText(L"textinput");
				fld->SetFontSize(32);
				btnSlot = box->Add(fld->MakeShared());
				btnSlot->fillX = true;
				btnSlot->padding = Margin(2);

				sld = new Slider(m_guiStyle);
				btnSlot = box->Add(sld->MakeShared());
				btnSlot->fillX = true;
				btnSlot->padding = Margin(2);

				sld = new Slider(m_guiStyle);
				btnSlot = box->Add(sld->MakeShared());
				btnSlot->fillX = true;
				btnSlot->padding = Margin(2);

				sld = new Slider(m_guiStyle);
				btnSlot = box->Add(sld->MakeShared());
				btnSlot->fillX = true;
				btnSlot->padding = Margin(2);
			}
		}

		// Setting bar
		{
			SettingsBar* sb = new SettingsBar(m_guiStyle);
			m_settings = Ref<SettingsBar>(sb);
			sb->AddSetting(&a, 0.0f, 1.0f, "A");
			sb->AddSetting(&b, 0.0f, 10.0f, "B");
			sb->AddSetting(&c, 0.0f, 5.0f, "C");
			sb->AddSetting(&d, -2.0f, 2.0f, "D");

			Canvas::Slot* slot = g_rootCanvas->Add(sb->MakeShared());
			slot->anchor = Anchor(0.75f, 0.0f, 1.0f, 1.0f);
			slot->autoSizeX = false;
			slot->autoSizeY = false;
		}

		// Spinner
		{
			Spinner* spinner = new Spinner(m_guiStyle);
			Canvas::Slot* slot = g_rootCanvas->Add(spinner->MakeShared());
			slot->anchor = Anchor(0.9f, 0.9f);
			slot->autoSizeX = true;
			slot->autoSizeY = true;
			slot->alignment = Vector2(1.0f, 1.0f);
		}
		return true;
	}
	~Test_Impl()
	{
	}
	virtual void OnKeyPressed(Key key) override
	{
		if(key == Key::Tab)
		{
			m_settings->SetShow(!m_settings->IsShown());
		}
	}
	virtual void OnKeyReleased(Key key) override
	{
	}
	virtual void Render(float deltaTime) override
	{
	}
	virtual void Tick(float deltaTime) override
	{
	}
};

Test* Test::Create()
{
	Test_Impl* impl = new Test_Impl();
	return impl;
}
