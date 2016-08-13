#include "stdafx.h"
#include "SettingsBar.hpp"
#include "Label.hpp"
#include "Slider.hpp"
#include "GUIRenderer.hpp"

SettingsBar::SettingsBar(Ref<CommonGUIStyle> style) : ScrollBox(style)
{
	m_style = style;
	m_container = new LayoutBox();
	m_container->layoutDirection = LayoutBox::Vertical;
	ScrollBox::SetContent(m_container->MakeShared());
}
SettingsBar::~SettingsBar()
{
	ClearSettings();
}

void SettingsBar::PreRender(GUIRenderData rd, GUIElementBase*& inputElement)
{
	rd.area = padding.Apply(rd.area);
	ScrollBox::PreRender(rd, inputElement);

	if(!m_shown)
	{
		GUIElementBase* dummy = nullptr;
		ScrollBox::PreRender(rd, dummy);
	}
	else
	{
		ScrollBox::PreRender(rd, inputElement);
	}
}

void SettingsBar::Render(GUIRenderData rd)
{
	if(m_shown)
	{
		rd.guiRenderer->RenderRect(rd.area, Color::White.WithAlpha(0.1f));
		rd.area = padding.Apply(rd.area);
		ScrollBox::Render(rd);
	}
}

void SettingsBar::AddSetting(float* target, float min, float max, const String& name)
{
	SettingBarSetting* setting = new SettingBarSetting();
	setting->name = Utility::ConvertToWString(name);
	setting->floatSetting.target = target;
	setting->floatSetting.min = min;
	setting->floatSetting.max = max;
	float v = (target[0] - min) / (max - min);

	LayoutBox* box = new LayoutBox();
	box->layoutDirection = LayoutBox::Vertical;
	LayoutBox::Slot* slot = m_container->Add(box->MakeShared());
	slot->fillX = true;

	{
		// The label
		Label* label = setting->label = new Label();
		box->Add(label->MakeShared());

		// The slider
		Slider* slider = new Slider(m_style);
		slider->SetValue(v);
		slider->OnSliding.Add(setting, &SettingBarSetting::m_SliderUpdate);
		LayoutBox::Slot* sliderSlot = box->Add(slider->MakeShared());
		sliderSlot->fillX = true;
	}

	m_settings.Add(setting, box->MakeShared());
	setting->m_SliderUpdate(v); // Initial update
}
void SettingsBar::ClearSettings()
{
	for(auto & s : m_settings)
	{
		delete s.first;
		m_container->Remove(s.second);
	}
	m_settings.clear();
}

void SettingsBar::SetShow(bool shown)
{
	m_shown = shown;
}

void SettingBarSetting::m_SliderUpdate(float val)
{
	floatSetting.target[0] = val * (floatSetting.max - floatSetting.min) + floatSetting.min;
	label->SetText(Utility::WSprintf(L"%s (%f):", name, floatSetting.target[0]));
}
