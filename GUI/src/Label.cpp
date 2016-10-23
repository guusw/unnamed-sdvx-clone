#include "stdafx.h"
#include "GUI.hpp"
#include "GUIRenderer.hpp"
#include "Label.hpp"
#include <Shared/Color.hpp>

void Label::Update(GUIUpdateData data)
{
	if(!m_transformValid.IsValid())
		m_textTransform.Invalidate();
	GUIElementBase::Update(data);
}
void Label::Render(GUIRenderData data)
{
	if(visibility != Visibility::Visible)
		return;

	m_UpdateText();
	Graphics::Text textObject = m_text.Get();

	// Update text transform?
	if(!m_textTransform.IsValid())
	{
		m_textTransform = Transform2D(m_area.pos) * m_renderTransform;
	}

	data.guiRenderer->RenderText(m_textTransform, textObject, color);

	// Render Debug
	if(data.debug)
	{
		Rect debug = Rect(textObject->textBounds.pos + m_area.pos, textObject->textBounds.size);
		data.guiRenderer->RenderWireBox(debug.ToTransform(), Color::FromHSV(99.0f, 0.6f, 0.8f));
		debug = Rect(m_area.pos, textObject->size);
		data.guiRenderer->RenderWireBox(debug.ToTransform(), Color::FromHSV(99.0f, 0.6f, 0.8f));
	}
}
Vector2 Label::m_GetDesiredBaseSize(GUIUpdateData data)
{
	m_UpdateText();
	if(!m_text.Get())
		return Vector2();

	return m_text.Get()->size;
}
void Label::m_UpdateText()
{
	if(m_text.IsValid())
		return;

	Graphics::Font fontToUse = font.Get();
	assert(fontToUse);
	m_text = fontToUse->CreateTextMonospaced(text, fontSize, monospacedWidth);
}