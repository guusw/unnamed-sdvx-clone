#include "stdafx.h"
#include "Button.hpp"
#include "GUIRenderer.hpp"

Button::Button(Ref<CommonGUIStyle> style)
{
	m_style = style;
}
void Button::m_OnPressed(bool isKeyboard)
{
	auto anim = AddAnimation(GUIAnimationUpdater::FromObject(this, &Button::Anim_Click), 0.2f);
}
void Button::m_OnReleased()
{
	auto anim = AddAnimation(GUIAnimationUpdater::FromObject(this, &Button::Anim_Click), 0.15f);
	anim->reversed = true;
}
void Button::Update(GUIUpdateData data)
{
	Margini padding = m_style->buttonBorder + (int32)m_animationPadding;
	data.area = padding.Apply(data.area);
	ButtonBase::Update(data);
}
void Button::Render(GUIRenderData rd)
{
	// Render BG
	Margini padding = (int32)m_animationPadding;
	rd.guiRenderer->RenderBorder(m_objectTransform, (m_hovered) ? m_style->buttonHighlightTexture : m_style->buttonTexture, m_style->buttonBorder);

	// Render content
	ButtonBase::Render(rd);
}
Vector2 Button::m_GetDesiredBaseSize(GUIUpdateData data)
{
	Vector2 baseSize = ButtonBase::GetDesiredSize(data);
	return baseSize + m_style->buttonBorder.GetSize() + m_style->buttonPadding.GetSize();
}
void Button::Anim_Click(float time)
{
	m_animationPadding = time * -2.0f;
	InvalidateArea();
}