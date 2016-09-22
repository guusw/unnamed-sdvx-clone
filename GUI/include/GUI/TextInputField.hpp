#pragma once
#include "LayoutBox.hpp"
#include "CommonGUIStyle.hpp"

/* Field that allows text input when focused */
class TextInputField : public LayoutBox
{
public:
	TextInputField(Ref<CommonGUIStyle> style);
	virtual void PreRender(GUIRenderData rd, GUIElementBase*& inputElement) override;
	virtual void Render(GUIRenderData rd) override;

	virtual Vector2 GetDesiredSize(GUIRenderData rd);

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
	Ref<CommonGUIStyle> m_style;

	Rect m_cachedInnerRect;
	Rect m_cachedTextRect;

	class Label* text;
	class Label* composition;

	float m_caretBlinkTimer = 0.0f;
	bool m_hovered = false;
};