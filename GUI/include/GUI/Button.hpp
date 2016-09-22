#pragma once
#include "GUIElement.hpp"
#include "CommonGUIStyle.hpp"

class Button : public GUIElementBase
{
public:
	Button(Ref<CommonGUIStyle> style);

	virtual void PreRender(GUIRenderData rd, GUIElementBase*& inputElement) override;
	virtual void Render(GUIRenderData rd) override;
	virtual Vector2 GetDesiredSize(GUIRenderData rd) override;

	// Set the text shown on the button
	void SetText(const WString& text);

	// The size(height) of the displayed text
	uint32 GetFontSize() const;
	void SetFontSize(uint32 size);

	// Called when pressed
	Delegate<> OnPressed;

private:
	bool m_hovered = false;
	bool m_held = true;
	bool m_dirty = false;
	bool m_animation = false;

	float m_animationPadding = 0.0f;

	Rect m_cachedInnerRect;

	Text m_text;
	WString m_textString;
	Ref<CommonGUIStyle> m_style;
	uint32 m_fontSize = 16;
};