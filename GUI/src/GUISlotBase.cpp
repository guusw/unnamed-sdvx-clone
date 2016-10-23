#include "stdafx.h"
#include "GUISlotBase.hpp"
#include "GUIRenderData.hpp"
#include "GUIRenderer.hpp"
#include <Shared/Color.hpp>

GUISlotBase::GUISlotBase()
{
	auto invalidateArea = &GUISlotBase::InvalidateArea;

	// Area changes
	padding.Notify.Bind(this, invalidateArea);
	allowOverflow.Notify.Bind(this, invalidateArea);
	alignment.Notify.Bind(this, invalidateArea);
	fillX.Notify.Bind(this, invalidateArea);
	fillY.Notify.Bind(this, invalidateArea);

	// Notify parent about ZOrder changes
	zOrder.Notify = [&]() {parent->m_OnChildZOrderChanged(this); };
}
GUISlotBase::~GUISlotBase()
{
	if(element)
	{
		element->m_OnRemovedFromParent();
	}
}

void GUISlotBase::Update(GUIUpdateData data)
{
	if(!m_cachedArea.IsValid())
	{
		Rect childRect = Rect(Vector2(), element->GetDesiredSize(data));
		data.area = ApplyAlignment(alignment, childRect, data.area);
		data.area = padding->Apply(data.area);
		m_UpdateArea(data.area);
	}
	else
	{
		data.area = m_cachedArea;
	}
	element->Update(data);
}
Vector2 GUISlotBase::GetDesiredSize(GUIUpdateData data)
{
	Vector2 size = element->m_GetDesiredBaseSize(data);
	size += padding->GetSize();
	return size;
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
void GUISlotBase::m_UpdateArea(Rect area)
{
	element->InvalidateArea();
	m_cachedArea.Update(area);
}
void GUISlotBase::InvalidateArea()
{
	if(m_cachedArea.IsValid())
	{
		m_cachedArea.Invalidate();
		// Notify parent
		parent->m_OnChildSlotChanged(this);
	}
}

const Vector2 GUISlotBase::MinusOne = Vector2(-1.0f);