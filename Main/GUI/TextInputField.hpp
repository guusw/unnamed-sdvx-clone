#pragma once
#include "LayoutBox.hpp"

/* Field that allows text input when focused */
class TextInputField : public LayoutBox
{
public:
	TextInputField();
	virtual void Render(GUIRenderData rd) override;

	// Set text of input field
	virtual void SetText(const WString& newText);
	virtual const WString& GetText() const;

	virtual void SetFontSize(int32 size);

	// Input field background color
	Color backgroundColor = Color::Blue.WithAlpha(0.3f);
	Color backgroundColorActive = Color::Blue.WithAlpha(0.5f);
	// Tet color
	Color color = Color::White;
	// Higlighted IME composition color
	Color compositionColor = Color::Green;

	// Called when text is updated
	Delegate<const WString&> OnTextUpdated;

protected:
	class Label* text;
	class Label* composition;
};