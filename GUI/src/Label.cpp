#include "stdafx.h"
#include "GUI.hpp"
#include "GUIRenderer.hpp"

// Label/Text Element
void Label::Render(GUIRenderData rd)
{
	rd.transform = m_renderTransform * rd.transform;

	m_TickAnimations(rd.deltaTime);

	if(visibility != Visibility::Visible)
		return;

	if(m_dirty)
		m_UpdateText(rd.guiRenderer);

	rd.area.pos = Vector2i(rd.area.pos);

	Transform2D transform = rd.transform * rd.area.pos;
	rd.guiRenderer->RenderText(transform, m_text, color);

	// Render Debug
	if(rd.debug)
	{
		Rect debug = Rect(m_text->textBounds.pos + rd.area.pos, m_text->textBounds.size);
		rd.guiRenderer->RenderWireBox(rd.transform * debug.ToTransform(), Color::FromHSV(99.0f, 0.6f, 0.8f));
	}
}
Vector2 Label::GetDesiredSize(GUIRenderData rd)
{
	if(visibility == Visibility::Collapsed)
		return Vector2();

	if(m_dirty)
		m_UpdateText(rd.guiRenderer);

	if(!m_text)
		return Vector2();

	return m_text->size;
}
void Label::SetText(const WString& text)
{
	if(m_textString == text)
		return; // No needless updates
	m_textString = text;
	m_dirty = true;
}
void Label::SetFont(Graphics::Font font)
{
	m_font = font;
}
void Label::SetMonospaced(float monospacedWidth /*= 0.0f*/)
{
	if(m_monospacedWidth != monospacedWidth)
	{
		m_monospacedWidth = monospacedWidth;
		m_dirty = true;
	}
}
const WString& Label::GetText() const
{
	return m_textString;
}
void Label::SetFontSize(uint32 size)
{
	m_fontSize = size;
	m_dirty = true;
}
uint32 Label::GetFontSize() const
{
	return m_fontSize;
}
void Label::m_UpdateText(class GUIRenderer* renderer)
{
	Graphics::Font fontToUse = m_font;
	if(!fontToUse)
		fontToUse = renderer->font;
	m_text = fontToUse->CreateTextMonospaced(m_textString, m_fontSize, m_monospacedWidth);
	m_dirty = false;
}