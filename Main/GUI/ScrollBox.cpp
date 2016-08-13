#include "stdafx.h"
#include "ScrollBox.hpp"
#include "GUIRenderer.hpp"
#include "Slider.hpp"

ScrollBox::ScrollBox(Ref<CommonGUIStyle> style)
{
	m_style = style;

	m_vscroll = new Slider(m_style);
	m_vscroll->layoutDirection = Slider::Vertical;
	m_vscroll->OnSliding.Add(this, &ScrollBox::m_OnSetScroll);
}

ScrollBox::~ScrollBox()
{
	delete m_vscroll;
	if(m_content)
	{
		delete m_content;
	}
}
void ScrollBox::PreRender(GUIRenderData rd, GUIElementBase*& inputElement)
{
	Rect sourceRect = rd.area;

	// Apply scroll wheel scroll
	m_hovered = rd.OverlapTest(sourceRect);
	if(m_content && m_hovered && !m_vscroll->IsHeld())
	{
		int32 mouseScroll = rd.guiRenderer->GetMouseScroll() * 64;
		if(mouseScroll != 0)
		{
			m_AnimateScrollDelta(mouseScroll);
		}
	}

	m_cachedSliderRect = Rect(Vector2(), m_vscroll->GetDesiredSize(rd));
	m_cachedContentRect = sourceRect;
	m_cachedContentRect.size.x -= m_cachedSliderRect.size.x;
	m_cachedSliderRect.pos = m_cachedContentRect.pos;
	m_cachedSliderRect.pos.x += m_cachedContentRect.size.x;
	m_cachedSliderRect.size.y = m_cachedContentRect.size.y;
	if(m_content)
	{
		m_cachedContentClipRect = m_cachedContentRect;

		/// Note: Maybe change way of getting desired size of vertical box containers
		rd.area.size.y = 100000000.0f;
		Vector2 contentSize = m_content->GetDesiredSize(rd);

		// Check for vertical overflow of content
		if(contentSize.y > sourceRect.size.y)
		{
			m_vscroll->showButton = true;
			m_cachedContentRect.size.y = contentSize.y;
		}
		else
		{
			// No scrolling required
			m_vscroll->showButton = false;
		}
	}

	if(m_content)
	{
		// Scissor rect for input
		rd.guiRenderer->PushScissorRect(m_cachedContentClipRect);
		rd.area = m_cachedContentRect;
		m_content->PreRender(rd, inputElement);
		rd.guiRenderer->PopScissorRect();
	}

	rd.area = m_cachedSliderRect;
	m_vscroll->PreRender(rd, inputElement);
}
void ScrollBox::Render(GUIRenderData rd)
{
	m_TickAnimations(rd.deltaTime);

	rd.area = m_cachedSliderRect;
	m_vscroll->Render(rd);

	if(m_content)
	{
		rd.area = m_cachedContentRect;
		rd.guiRenderer->PushScissorRect(m_cachedContentClipRect);
		m_content->Render(rd);
		rd.guiRenderer->PopScissorRect();
	}
}
Vector2 ScrollBox::GetDesiredSize(GUIRenderData rd)
{
	if(m_content)
	{
		Vector2 contentSize = m_content->GetDesiredSize(rd);

		// Add scroll bar size on x axis
		contentSize.x += m_vscroll->GetDesiredSize(rd).x;

		return contentSize;
	}
	return Vector2();
}
ScrollBox::Slot* ScrollBox::SetContent(GUIElement content)
{
	if(m_content)
	{
		delete m_content;
		m_content = nullptr;
	}
	if(content)
	{
		m_content = CreateSlot<Slot>(content);
		m_vscroll->SetValue(0);
	}
	return m_content;
}
ScrollBox::Slot* ScrollBox::GetContentSlot()
{
	return m_content;
}

void ScrollBox::m_AnimateScrollDelta(int32 delta)
{
	Ref<IGUIAnimation> currentAnimation = GetAnimation(0u);
	if(currentAnimation)
	{
		// Apply scroll to target for current animation
		m_scrollTarget = m_scrollTarget + delta;
	}
	else
	{
		m_scrollTarget = m_content->GetScroll() + delta;
	}
	Ref<IGUIAnimation> anim = IGUIAnimation::CreateCallback(m_scrollTarget, m_content->GetScroll(), 0.1f,
		[&](int32 scroll)
	{
		m_content->SetScroll(scroll);
		m_vscroll->SetValue(m_content->GetScrollPercent(), false);
	}, 0);
	anim->timeFunction = TimeFunction::EaseOutQuad;
	AddAnimation(anim, true);
}
void ScrollBox::m_OnSetScroll(float val)
{
	if(m_content)
	{
		m_scrollTarget = m_content->ConvertFromPercent(val);
		AddAnimation(IGUIAnimation::CreateCallback(m_scrollTarget, m_content->GetScroll(), 0.1f,
			[&](int32 scroll)
		{
			m_content->SetScroll(scroll);
		}, 0), true);
	}
}
void ScrollBox::Slot::PreRender(GUIRenderData rd, GUIElementBase*& inputElement)
{
	// Apply scrolling
	ScrollBox* sb = (ScrollBox*)parent;
	rd.area.pos.y -= m_scroll;
	m_cachedArea = rd.area;
	element->PreRender(rd, inputElement);
}
void ScrollBox::Slot::Render(GUIRenderData rd)
{
	rd.area = m_cachedArea;
	element->Render(rd);
}
void ScrollBox::Slot::SetScrollPercent(float percent)
{
	// Apply scrolling
	ScrollBox* sb = (ScrollBox*)parent;
	Vector2 overflow = sb->m_cachedContentRect.size - sb->m_cachedContentClipRect.size;
	if(overflow.y > 0.0f)
	{
		int32 pixelOffset = (int32)(percent * overflow.y);
		SetScroll(pixelOffset);
	}
	else
	{
		SetScroll(0);
	}
}
void ScrollBox::Slot::SetScroll(int32 pixelOffset)
{
	// Apply scrolling
	ScrollBox* sb = (ScrollBox*)parent;
	Vector2 overflow = sb->m_cachedContentRect.size - sb->m_cachedContentClipRect.size;
	int32 newScroll;
	if(overflow.y > 0.0f)
	{
		newScroll = Math::Clamp(pixelOffset, 0, (int32)overflow.y);
	}
	else
	{
		newScroll = 0;
	}
	m_scroll = newScroll;
}

int32 ScrollBox::Slot::ConvertFromPercent(float percent)
{
	ScrollBox* sb = (ScrollBox*)parent;
	Vector2 overflow = sb->m_cachedContentRect.size - sb->m_cachedContentClipRect.size;
	if(overflow.y > 0.0f)
	{
		return (int32)(percent * overflow.y);
	}
	else
	{
		return 0;
	}
}

float ScrollBox::Slot::GetScrollPercent() const
{
	ScrollBox* sb = (ScrollBox*)parent;
	Vector2 overflow = sb->m_cachedContentRect.size - sb->m_cachedContentClipRect.size;
	if(overflow.y > 0.0f)
	{
		return (float)m_scroll / overflow.y;
	}
	else
	{
		return 0.0f;
	}
}
