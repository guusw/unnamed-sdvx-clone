#pragma once
#include <GUI/GUIElement.hpp>
#include <Graphics/Font.hpp>
#include <Shared/Color.hpp>

class Label : public GUIElementBase
{
public:
	void Update(GUIUpdateData data) override;
	void Render(GUIRenderData data) override;

	// The text displayed
	NotifyDirty<WString> text;
	// Sets font to use, overriding the default font
	NotifyDirty<Graphics::Font> font;
	// The size(height) of the displayed text
	NotifyDirty<int32> fontSize = 32;
	// If this is larger than 0, this value is used as the spacing between glyphs
	NotifyDirty<float> monospacedWidth = 0.0f;

	// Color of the text
	Color color = Color::White;

protected:
	Vector2 m_GetDesiredBaseSize(GUIUpdateData data) override;

private:
	void m_UpdateText();

	Cached<Transform2D> m_textTransform;

	// Text object that is displayed
	Cached<Graphics::Text> m_text;
};
