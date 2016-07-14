#pragma once
#include "GUIElement.hpp"

class Label : public GUIElementBase
{
public:
	void Render(GUIRenderData rd) override;
	bool GetDesiredSize(GUIRenderData rd, Vector2& sizeOut) override;

	// The text displayed
	const WString& GetText() const;
	void SetText(const WString& text);

	// The size(height) of the displayed text
	uint32 GetFontSize() const;
	void SetFontSize(uint32 size);

	// Color of the text
	Color color = Color::White;

private:
	void m_UpdateText(class GUIRenderer* renderer);

	bool m_dirty = true;
	// Text object that is displayed
	Text m_text;
	// Text string that is displayed
	WString m_textString;
	// Font override
	Font m_font;
	uint32 m_fontSize = 16;
};