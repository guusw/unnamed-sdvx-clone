#include "stdafx.h"
#include "TextInputField.hpp"
#include "Label.hpp"
#include "GUIRenderer.hpp"

TextInputField::TextInputField()
{
	layoutDirection = LayoutBox::Horizontal;

	text = new Label();
	Slot* slot = Add(text->MakeShared());
	slot->fill = false;
	text->SetText(L"");
	text->SetFontSize(32);

	composition = new Label();
	composition->color = compositionColor;
	slot = Add(composition->MakeShared());
	slot->fill = false;
	composition->SetText(L"");
	composition->SetFontSize(32);
}
void TextInputField::Render(GUIRenderData rd)
{
	rd.guiRenderer->RenderRect(*rd.rq, rd.area, HasInputFocus() ? backgroundColorActive : backgroundColor);

	if(HasInputFocus())
	{
		const GUITextInput& input = rd.guiRenderer->GetTextInput();
		WString newText = input.Apply(GetText());
		SetText(newText);

		// Update IME String
		composition->visibility = (input.composition.empty()) ? Visibility::Collapsed : Visibility::Visible;
		composition->SetText(input.composition);
	}
	else
	{
		composition->visibility = Visibility::Collapsed;
	}

	text->color = color;
	composition->color = compositionColor;

	LayoutBox::Render(rd);
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
