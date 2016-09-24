#pragma once
#include "GUIElement.hpp"
#include "CommonGUIStyle.hpp"

class Slider : public GUIElementBase
{
public:
	Slider(Ref<CommonGUIStyle> style);

	virtual void PreRender(GUIRenderData rd, GUIElementBase*& inputElement) override;
	virtual void Render(GUIRenderData rd) override;
	virtual Vector2 GetDesiredSize(GUIRenderData rd) override;

	float GetValue() const;
	void SetValue(float val, bool callHandlers = true);

	bool IsHeld() const { return m_held; }

	enum LayoutDirection
	{
		Horizontal,
		Vertical
	};

	// If slider button should be shown and interactible
	bool showButton = true;

	// The layout direction of the box
	LayoutDirection layoutDirection = LayoutDirection::Horizontal;

	// Called when sliding (or set)
	Delegate<float> OnSliding;
	// Called when value is set by code or by releasing the slider
	Delegate<float> OnValueSet;

private:
	Rect m_CalculateButtonRect(float inVal);

	bool m_hovered = false;
	bool m_held = false;
	bool m_animation = false;
	Vector2 m_dragStart;

	Rect m_cachedButtonParentRect;
	Rect m_cachedButtonRect;
	Rect m_cachedSliderRect;

	float m_buttonScale = 1.0f;
	float m_newValue = 0.0f;
	float m_value = 0.0f;

	Ref<CommonGUIStyle> m_style;
};