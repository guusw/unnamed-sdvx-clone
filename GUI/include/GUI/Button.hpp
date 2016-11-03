#pragma once
#include <GUI/GUISlotBase.hpp>
#include <GUI/CommonGUIStyle.hpp>
#include <GUI/ButtonBase.hpp>

/*
	Common style button
*/
class Button : public ButtonBase
{
public:
	Button(Ref<CommonGUIStyle> style);

	virtual void Update(GUIUpdateData data) override;
	virtual void Render(GUIRenderData data) override;

protected:
	void Anim_Click(float time);
	virtual void m_OnReleased() override;
	virtual void m_OnPressed(bool isKeyboard) override;
	Vector2 m_GetDesiredBaseSize(GUIUpdateData data) override;

	float m_animationPadding;
	Ref<CommonGUIStyle> m_style;
};
