#pragma once
#include "GUIElement.hpp"

class Label : public GUIElementBase
{
public:
	void Render(GUIRenderData rd) override;
	Vector2 GetDesiredSize(GUIRenderData rd) override;

	// The text displayed
	const WString& GetText() const;
	void SetText(const WString& text);

	// Sets font to use, overriding the default font
	void SetFont(Graphics::Font font);
	
	void SetTextOptions(FontRes::TextOptions options);
	FontRes::TextOptions GetTextOptions() const;

	// The size(height) of the displayed text
	uint32 GetFontSize() const;
	void SetFontSize(uint32 size);

	// Color of the text
	Color color = Color::White;

private:
	void m_UpdateText(class GUIRenderer* renderer);

	bool m_dirty = true;
	// Special text options
	FontRes::TextOptions m_textOptions = FontRes::None;
	// Text object that is displayed
	Text m_text;
	// Text string that is displayed
	WString m_textString;
	// Font override
	Graphics::Font m_font;
	uint32 m_fontSize = 16;
};