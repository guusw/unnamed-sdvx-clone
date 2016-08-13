#include "stdafx.h"
#include "TextInputField.hpp"
#include "Label.hpp"
#include "GUIRenderer.hpp"

TextInputField::TextInputField(Ref<CommonGUIStyle> style)
{
	m_style = style;
	layoutDirection = LayoutBox::Horizontal;

	text = new Label();
	Slot* slot = Add(text->MakeShared());
	text->SetText(L"");
	text->SetFontSize(32);

	composition = new Label();
	composition->color = compositionColor;
	slot = Add(composition->MakeShared());
	composition->SetText(L"");
	composition->SetFontSize(32);
}

void TextInputField::PreRender(GUIRenderData rd, GUIElementBase*& inputElement)
{
	Rect inner = m_style->textfieldBorder.Apply(rd.area);
	m_hovered = rd.OverlapTest(inner);

	if(m_hovered)
		inputElement = this;

	Rect inner1 = m_style->textfieldPadding.Apply(inner);
	rd.area = inner1;
	m_cachedInnerRect = rd.area;

	// Clip text to inner
	m_cachedTextRect = Rect(Vector2(), LayoutBox::GetDesiredSize(rd));
	if(m_cachedTextRect.size.x > m_cachedInnerRect.size.x)
	{
		m_cachedTextRect = GUISlotBase::ApplyAlignment(Vector2(1.0f, 0.0f), m_cachedTextRect, m_cachedInnerRect);
		rd.area = m_cachedTextRect;
	}
	else
	{
		// No clipping required text just fits in the box
		rd.area = m_cachedTextRect = m_cachedInnerRect;
	}
	LayoutBox::PreRender(rd, inputElement);
}
void TextInputField::Render(GUIRenderData rd)
{
	rd.guiRenderer->RenderButton(rd.area, HasInputFocus() ? m_style->textfieldHighlightTexture : m_style->textfieldTexture,
		m_style->textfieldBorder, m_hovered ? Color::White : Color(0.7f));

	if(m_hovered && rd.guiRenderer->GetMouseButtonPressed(MouseButton::Left))
	{
		rd.guiRenderer->SetInputFocus(this);
	}

	if(HasInputFocus())
	{
		const GUITextInput& input = rd.guiRenderer->GetTextInput();
		WString newText = input.Apply(GetText());
		SetText(newText);

		// Update IME String
		composition->visibility = (input.composition.empty()) ? Visibility::Collapsed : Visibility::Visible;
		composition->SetText(input.composition);

		// Draw caret
		float caretOffset = text->GetDesiredSize(rd).x + composition->GetDesiredSize(rd).x;
		caretOffset += m_cachedTextRect.pos.x - m_cachedInnerRect.pos.x;
		Vector2 caretSize = Vector2(0, (float)text->GetFontSize());
		caretSize.x = caretSize.y * 0.1f;
		m_caretBlinkTimer += rd.deltaTime;
		int32 caretBlink = (int32)(m_caretBlinkTimer * 2) % 2;
		Rect caretRect = Rect(m_cachedInnerRect.pos + Vector2(caretOffset, 0.0f), caretSize);
		// Center vertically
		caretRect.pos.y += (m_cachedInnerRect.size.y - caretSize.y) * 0.5f;
		rd.guiRenderer->RenderRect(caretRect, Color::White.WithAlpha((caretBlink == 0) ? 1.0f : 0.2f), Texture());
	}
	else
	{
		composition->visibility = Visibility::Collapsed;
	}

	text->color = color;
	composition->color = compositionColor;

	rd.guiRenderer->PushScissorRect(m_cachedInnerRect);
	rd.area = m_cachedTextRect;
	LayoutBox::Render(rd);
	rd.guiRenderer->PopScissorRect();
}

Vector2 TextInputField::GetDesiredSize(GUIRenderData rd)
{
	Vector2 sizeOut = LayoutBox::GetDesiredSize(rd);
	sizeOut += m_style->buttonBorder.GetSize() + m_style->buttonPadding.GetSize();

	// Don't let horizontal size exceed the maximum area given
	if(sizeOut.x > rd.area.size.x)
		sizeOut.x = rd.area.size.x;

	return sizeOut;
}

void TextInputField::SetText(const WString& newText)
{
	if(newText != text->GetText())
	{
		text->SetText(newText);
		OnTextUpdated.Call(newText);
	}
}
const WString& TextInputField::GetText() const
{
	return text->GetText();
}
void TextInputField::SetFontSize(int32 size)
{
	text->SetFontSize(size);
	composition->SetFontSize(size);
}
