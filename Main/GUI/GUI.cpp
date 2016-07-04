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

	// Apply filling
	Vector2 size;
	if(GetDesiredSize(rd,size))
	{
		rd.area = ApplyFill(size, rd.area);
	}

	element->Render(rd);
}
bool GUISlotBase::GetDesiredSize(GUIRenderData rd, Vector2& sizeOut)
{
	return element->GetDesiredSize(rd, sizeOut);
}
Rect GUISlotBase::ApplyFill(const Vector2& inSize, const Rect& rect)
{
	return ApplyFill(fillMode, inSize, rect);
}
Rect GUISlotBase::ApplyFill(FillMode fillMode, const Vector2& inSize, const Rect& rect)
{
	if(fillMode == FillMode::None)
		return Rect(rect.pos, inSize);
	else if(fillMode == FillMode::Stretch)
		return rect;
	else if(fillMode == FillMode::Fit)
	{
		float rx = inSize.x / rect.size.x;
		float ry = inSize.y / rect.size.y;
		float scale = 1.0f;
		if(rx > ry)
		{
			if(rx > 1.0f)
			{
				scale = 1.0f / rx;
			}
		}
		else // ry is largest
		{
			if(ry > 1.0f)
			{
				scale = 1.0f / ry;
			}
		}

		Rect ret = rect;
		if(scale < 1.0f)
		{
			ret.size = inSize * scale;
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
		if(scale > 1.0f)
		{
			Vector2 newSize = inSize * scale;
			Vector2 rem = rect.size - newSize;
			//ret.pos += rem * 0.5f;
			ret.size = newSize;
		}
		return ret;
	}
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
