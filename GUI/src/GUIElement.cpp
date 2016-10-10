#include "stdafx.h"
#include "GUI.hpp"
#include "GUIRenderer.hpp"

GUIElementBase::~GUIElementBase()
{
	if(m_rendererFocus)
	{
		m_rendererFocus->SetInputFocus(nullptr);
	}
}
void GUIElementBase::PreRender(GUIRenderData rd, GUIElementBase*& inputElement)
{
}
Vector2 GUIElementBase::GetDesiredSize(GUIRenderData rd)
{
	return Vector2();
}
bool GUIElementBase::AddAnimation(Ref<IGUIAnimation> anim, bool removeOld)
{
	void* target = anim->GetTarget();
	if(m_animationMap.Contains(target))
	{
		if(!removeOld)
			return false;
		m_animationMap.erase(target);
	}

	m_animationMap.Add(target, anim);
	return true;
}

Ref<IGUIAnimation> GUIElementBase::GetAnimation(uint32 uid)
{
	size_t suid = (size_t)uid;
	return GetAnimation((void*)suid);
}
Ref<IGUIAnimation> GUIElementBase::GetAnimation(void* target)
{
	Ref<IGUIAnimation>* found = m_animationMap.Find(target);
	if(found)
		return *found;
	return Ref<IGUIAnimation>();
}

bool GUIElementBase::HasInputFocus() const
{
	return m_rendererFocus != nullptr;
}

bool GUIElementBase::OverlapTest(Rect rect, Vector2 point)
{
	if(point.x < rect.Left() || point.x > rect.Right())
		return false;
	if(point.y < rect.Top() || point.y > rect.Bottom())
		return false;
	return true;
}

void GUIElementBase::m_OnRemovedFromParent()
{
	slot = nullptr;
}
void GUIElementBase::m_AddedToSlot(GUISlotBase* slot)
{
}
void GUIElementBase::m_OnZOrderChanged(GUISlotBase* slot)
{
}
void GUIElementBase::m_TickAnimations(float deltaTime)
{
	for(auto it = m_animationMap.begin(); it != m_animationMap.end();)
	{
		bool done = !it->second->Update(deltaTime);
		if(done)
		{
			it = m_animationMap.erase(it);
			continue;
		}
		it++;
	}
}

const Transform2D& GUIElementBase::GetRenderTransform() const
{
	return m_renderTransform;
}
void GUIElementBase::SetRenderTransform(const Transform2D& transform)
{
	m_renderTransform = transform;
}

GUISlotBase::~GUISlotBase()
{
	if(element)
	{
		element->m_OnRemovedFromParent();
	}
}
void GUISlotBase::PreRender(GUIRenderData rd, GUIElementBase*& inputElement)
{
	// Apply padding
	Rect scissorRect = rd.area = padding.Apply(rd.area);

	// Store area
	m_cachedArea = rd.area;

	element->PreRender(rd, inputElement);
}

void GUISlotBase::Render(GUIRenderData rd)
{
	// Scissor Rectangle
	rd.area = m_cachedArea;

	rd.guiRenderer->PushScissorRect(rd.area);
	element->Render(rd);
	rd.guiRenderer->PopScissorRect();
}

Vector2 GUISlotBase::GetDesiredSize(GUIRenderData rd)
{
	Vector2 size = element->GetDesiredSize(rd);
	return size + padding.GetSize();
}
Rect GUISlotBase::ApplyFill(FillMode fillMode, const Vector2& inSize, const Rect& rect)
{
	if(fillMode == FillMode::None)
	{
		return Rect(rect.pos, inSize);
	}
	else if(fillMode == FillMode::Stretch)
		return rect;
	else if(fillMode == FillMode::Fit)
	{
		float rx = inSize.x / rect.size.x;
		float ry = inSize.y / rect.size.y;
		float scale = 1.0f;
		if(rx > ry)
		{
			{
				scale = 1.0f / rx;
			}
		}
		else // ry is largest
		{
			{
				scale = 1.0f / ry;
			}
		}

		Rect ret = rect;
		{
			Vector2 newSize = inSize * scale;
			ret.size = newSize;
		}
		return ret;
	}
	else // Fill
	{
		float rx = inSize.x / rect.size.x;
		float ry = inSize.y / rect.size.y;
		float scale = 1.0f;
		if(rx < ry)
		{
			scale = 1.0f / rx;
		}
		else // ry is smallest
		{
			scale = 1.0f / ry;
		}

		Rect ret = rect;
		{
			Vector2 newSize = inSize * scale;
			ret.size = newSize;
		}
		return ret;
	}
}
Rect GUISlotBase::ApplyAlignment(const Vector2& alignment, const Rect& rect, const Rect& parent)
{
	Vector2 remaining = parent.size - rect.size;
	remaining.x = remaining.x;
	remaining.y = remaining.y;

	return Rect(parent.pos + remaining * alignment, rect.size);
}

void GUISlotBase::SetZOrder(int32 zorder)
{
	m_zorder = zorder;
	parent->m_OnZOrderChanged(this);
}
int32 GUISlotBase::GetZOrder() const
{
	return m_zorder;
}