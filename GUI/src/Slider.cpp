#include "stdafx.h"
#include "Slider.hpp"
#include "GUIRenderer.hpp"

Slider::Slider(Ref<CommonGUIStyle> style)
{
	m_style = style;
}
void Slider::PreRender(GUIRenderData rd, GUIElementBase*& inputElement)
{
	Rect baseSize = rd.area;
	if(layoutDirection == LayoutDirection::Horizontal)
	{
		baseSize.size.y = (float)m_style->sliderTexture->GetSize().y;
		baseSize = GUISlotBase::ApplyAlignment(Vector2(0.0f, 0.5f), baseSize, rd.area);
		m_cachedButtonRect = Rect(Vector2(), m_style->sliderButtonTexture->GetSize());
		m_cachedButtonRect.size *= m_buttonScale;
		m_cachedButtonParentRect = m_style->sliderButtonPadding.Apply(rd.area);
	}
	else
	{
		baseSize.size.x = (float)m_style->verticalSliderTexture->GetSize().x;
		baseSize = GUISlotBase::ApplyAlignment(Vector2(0.5f, 0.0f), baseSize, rd.area);
		m_cachedButtonRect = Rect(Vector2(), m_style->verticalSliderButtonTexture->GetSize());
		m_cachedButtonRect.size *= m_buttonScale;
		m_cachedButtonParentRect = m_style->verticalSliderButtonPadding.Apply(rd.area);
	}
	

	m_cachedButtonRect = m_CalculateButtonRect(m_value);

	//bool lastHovered = m_hovered;
	//m_hovered = rd.OverlapTest(m_cachedButtonRect);
	//if(m_hovered && rd.guiRenderer->GetMouseButtonPressed(MouseButton::Left) && showButton)
	//{
	//	m_held = true;
	//	m_dragStart = rd.guiRenderer->GetMousePos();
	//	rd.guiRenderer->SetInputFocus(nullptr);
	//}
	//
	//if(!m_animation)
	//{
	//	if(m_hovered || m_held)
	//	{
	//		AddAnimation(Ref<IGUIAnimation>(
	//			new GUIAnimation<float>(&m_buttonScale, 1.1f, 0.2f)), true);
	//		m_animation = true;
	//	}
	//}
	//else 
	//{
	//	if(!m_hovered && !m_held)
	//	{
	//		AddAnimation(Ref<IGUIAnimation>(
	//			new GUIAnimation<float>(&m_buttonScale, 1.0f, 0.3f)), true);
	//		m_animation = false;
	//	}
	//}


	//if(m_held)
	//{
	//	if(rd.guiRenderer->GetMouseButtonReleased(MouseButton::Left) || !showButton)
	//	{
	//		OnSliding.Call(m_newValue);
	//		OnValueSet.Call(m_newValue);
	//		m_value = m_newValue;
	//		m_held = false;
	//		m_cachedButtonRect = m_CalculateButtonRect(m_value);
	//	}
	//	else
	//	{
	//		// Set input element
	//		inputElement = this;
	//	}
	//}

	m_cachedSliderRect = baseSize;
}
void Slider::Render(GUIRenderData rd)
{
	m_TickAnimations(rd.deltaTime);

	float actualValue = m_value;
	Rect buttonRect = m_cachedButtonRect;

	if(m_held)
	{
		Vector2 mouseDelta = rd.guiRenderer->GetMousePos() - m_dragStart;

		float delta = 0.0f;
		if(layoutDirection == LayoutDirection::Horizontal)
		{
			float scrollableSize = m_cachedSliderRect.size.x - (float)m_style->sliderButtonTexture->GetSize().x;
			delta = mouseDelta.x / scrollableSize;
		}
		else
		{
			float scrollableSize = m_cachedSliderRect.size.y - (float)m_style->verticalSliderButtonTexture->GetSize().y;
			delta = mouseDelta.y / scrollableSize;
		}
		

		m_newValue = Math::Clamp(m_value + delta, 0.0f, 1.0f);
		OnSliding.Call(m_newValue);
		buttonRect = m_CalculateButtonRect(m_newValue);
	}

	if(layoutDirection == LayoutDirection::Horizontal)
	{
		rd.guiRenderer->RenderButton(m_cachedSliderRect, m_style->sliderTexture, m_style->sliderBorder);
		if(showButton)
		{
			rd.guiRenderer->RenderRect(buttonRect, Color::White,
				(m_hovered || m_held) ? m_style->sliderButtonHighlightTexture : m_style->sliderButtonTexture);
		}
	}
	else
	{
		rd.guiRenderer->RenderButton(m_cachedSliderRect, m_style->verticalSliderTexture, m_style->verticalSliderBorder);
		if(showButton)
		{
			rd.guiRenderer->RenderRect(buttonRect, Color::White,
				(m_hovered || m_held) ? m_style->verticalSliderButtonHighlightTexture : m_style->verticalSliderButtonTexture);
		}
	}
}
Vector2 Slider::GetDesiredSize(GUIRenderData rd)
{
	if(layoutDirection == LayoutDirection::Horizontal)
	{
		Vector2 buttonSize = m_style->sliderButtonTexture->GetSize();
		Vector2 sliderSize = m_style->sliderTexture->GetSize();
		return Vector2(buttonSize.x, Math::Max(buttonSize.y, sliderSize.y));
	}
	else
	{
		Vector2 buttonSize = m_style->verticalSliderButtonTexture->GetSize();
		Vector2 sliderSize = m_style->verticalSliderTexture->GetSize();
		return Vector2(Math::Max(buttonSize.x, sliderSize.x), buttonSize.y);
	}
	
}
float Slider::GetValue() const
{
	return m_value;
}
void Slider::SetValue(float val, bool callHandlers)
{
	m_value = val;
	if(callHandlers)
	{
		OnSliding.Call(m_newValue);
		OnValueSet.Call(m_value);
	}
}

Rect Slider::m_CalculateButtonRect(float inVal)
{
	if(layoutDirection == LayoutDirection::Horizontal)
	{
		return GUISlotBase::ApplyAlignment(Vector2(inVal, 0.5f), m_cachedButtonRect, m_cachedButtonParentRect);
	}
	else
	{
		return GUISlotBase::ApplyAlignment(Vector2(0.5f, inVal), m_cachedButtonRect, m_cachedButtonParentRect);
	}
}
