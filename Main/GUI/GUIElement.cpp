#include "stdafx.h"
#include "GUI.hpp"
#include "GUIRenderer.hpp"

void GUIElementBase::Render(GUIRenderData rd)
{
	m_TickAnimations(rd.deltaTime);
}
bool GUIElementBase::GetDesiredSize(GUIRenderData rd, Vector2& sizeOut)
{
	return false;
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
GUISlotBase::~GUISlotBase()
{
	if(element)
	{
		element->m_OnRemovedFromParent();
	}
}
void GUISlotBase::Render(GUIRenderData rd)
{
	// Apply padding
	Rect scissorRect = rd.area = padding.Apply(rd.area);

	// Scissor Rectangle
	rd.guiRenderer->SetScissorRect(scissorRect);

	element->Render(rd);
}
bool GUISlotBase::GetDesiredSize(GUIRenderData rd, Vector2& sizeOut)
{
	if(element->GetDesiredSize(rd, sizeOut))
	{
		sizeOut.x += padding.Width();
		sizeOut.y += padding.Height();
		return true;
	}
	return false;
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
	//remaining.x = Math::Max<float>(0, remaining.x);
	//remaining.y = Math::Max<float>(0, remaining.y);
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
