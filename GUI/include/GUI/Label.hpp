#pragma once
#include "GUIElement.hpp"
#include "Graphics/Font.hpp"

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

	void SetMonospaced(float monospacedWidth = 0.0f);

	// The size(height) of the displayed text
	uint32 GetFontSize() const;
	void SetFontSize(uint32 size);

	// Color of the text
	Color color = Color::White;

private:
	void m_UpdateText(class GUIRenderer* renderer);

	bool m_dirty = true;
	float m_monospacedWidth = 0.0f;
	// Text object that is displayed
	Graphics::Text m_text;
	// Text string that is displayed
	WString m_textString;
	// Font override
	Graphics::Font m_font;
	uint32 m_fontSize = 16;
};