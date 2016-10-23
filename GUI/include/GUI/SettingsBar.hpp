#pragma once
#include <GUI/GUIElement.hpp>
#include <GUI/LayoutBox.hpp>
#include <GUI/ScrollBox.hpp>
#include <GUI/CommonGUIStyle.hpp>

struct SettingBarSetting
{
	enum class Type
	{
		Float,
		Text,
		Button
	};
	Type type = Type::Float;
	union
	{
		struct 
		{
			float* target;
			float min;
			float max;
		} floatSetting;
	};
	WString name;
	class Label* label;

protected:
	friend class SettingsBar;
	void m_SliderUpdate(float val);
};

class SettingsBar : public ScrollBox
{
public:
	SettingsBar(Ref<CommonGUIStyle> style);
	~SettingsBar();

	virtual void PreRender(GUIRenderData rd, GUIElementBase*& inputElement) override;
	virtual void Render(GUIRenderData data) override;

	void AddSetting(float* target, float min, float max, const String& name);
	void ClearSettings();
	
	void SetShow(bool shown);
	bool IsShown() const { return m_shown; }

	Margini padding = Margini(5, 5, 0, 5);

private:
	bool m_shown = true;

	class LayoutBox* m_container;
	Ref<CommonGUIStyle> m_style;
	Map<SettingBarSetting*, GUIElement> m_settings;
};