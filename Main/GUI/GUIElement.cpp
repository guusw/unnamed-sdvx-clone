#include "stdafx.h"
#include "GUI.hpp"
#include "GUIRenderer.hpp"

void GUIElementBase::Render(GUIRenderData rd)
{
}
bool GUIElementBase::GetDesiredSize(GUIRenderData rd, Vector2& sizeOut)
{
	return false;
}
void GUIElementBase::m_AddedToSlot(GUISlotBase* slot)
{
}
void GUIElementBase::m_OnZOrderChanged(GUISlotBase* slot)
{
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
	return element->GetDesiredSize(rd, sizeOut);
}
Rect GUISlotBase::ApplyFill(FillMode fillMode, const Vector2& inSize, const Rect& rect)
{
	if(fillMode == FillMode::Stretch)
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
			Vector2 rem = rect.size - newSize;
			ret.pos += rem * 0.5f;
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
			Vector2 rem = rect.size - newSize;
			ret.pos += rem * 0.5f;
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
