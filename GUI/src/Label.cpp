#include "stdafx.h"
#include "GUI.hpp"
#include "GUIRenderer.hpp"

// Label/Text Element
void Label::Render(GUIRenderData rd)
{
	m_TickAnimations(rd.deltaTime);

	if(visibility != Visibility::Visible)
		return;

	if(m_dirty)
		m_UpdateText(rd.guiRenderer);

	rd.area.pos = Vector2i(rd.area.pos);

	/// DEBUG label draw area
	//rd.guiRenderer->RenderRect(rd.area, Color(1, 1, 1, 0.1f));

	rd.guiRenderer->RenderText(m_text, rd.area.pos, color);
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
void Label::SetTextOptions(FontRes::TextOptions options)
{
	if(m_textOptions != options)
		m_dirty = true;
	m_textOptions = options;
}
Graphics::FontRes::TextOptions Label::GetTextOptions() const
{
	return m_textOptions;
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
	m_text = fontToUse->CreateText(m_textString, m_fontSize, m_textOptions);
	m_dirty = false;
}