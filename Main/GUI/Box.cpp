#include "stdafx.h"
#include "GUI.hpp"
#include "GUIRenderer.hpp"

// Vertical/Horizontal Boxes

void VerticalBox::Render(GUIRenderData rd)
{
	Rect sourceRect = rd.area;
	float yOffset = 0.0f;
	float mult = 1.0f/(float)m_children.size();
	for(auto it = m_children.begin(); it != m_children.end(); it++)
	{
		Vector2 elemSize;
		if(fill)
		{
			elemSize = sourceRect.size;
			elemSize.y = sourceRect.size.y * mult; // Equal size
			rd.area.pos = sourceRect.pos;
			rd.area.pos.y += yOffset;
			rd.area.size = elemSize;
		}
		else
		{
			float sizeLeft = Math::Max(0.0f, sourceRect.size.y - yOffset);

			rd.area = sourceRect;

			(*it)->GetDesiredSize(rd, elemSize);
			elemSize.y = Math::Min(elemSize.y, sizeLeft);

			rd.area.pos.y += yOffset;
			rd.area.size.y = elemSize.y;
		}

		(*it)->Render(rd);

		yOffset += elemSize.y;
	}
}
bool VerticalBox::GetDesiredSize(GUIRenderData rd, Vector2& sizeOut)
{
	sizeOut = Vector2(0, 0);
	bool set = false;
	for(auto it = m_children.begin(); it != m_children.end(); it++)
	{
		Vector2 elemSize;
		if((*it)->GetDesiredSize(rd,elemSize))
		{
			sizeOut.y += elemSize.y;
			sizeOut.x = Math::Max(sizeOut.x, elemSize.x);
			set = true;
		}
	}
	return set;
}
void VerticalBox::Add(GUIElement element)
{
	bool found = false;
	for(auto it = m_children.begin(); it != m_children.end(); it++)
	{
		if((*it)->element == element)
			return; // Already exists
	}

	m_children.AddUnique(CreateSlot<VerticalBox::Slot>(element));
}
void VerticalBox::Remove(GUIElement element)
{
	for(auto it = m_children.begin(); it != m_children.end();)
	{
		if((*it)->element == element)
			m_children.erase(it);
		else
			it++;
	}
}
const Vector<VerticalBox::Slot*>& VerticalBox::GetChildren()
{
	return m_children;
}

void BoxSlotBase::Render(GUIRenderData rd)
{
	// Apply padding
	Rect scissorRect = rd.area = padding.Apply(rd.area);

	// Scissor Rectangle
	rd.guiRenderer->SetScissorRect(scissorRect);

	// Apply filling
	Vector2 size = rd.area.size;
	if(GetDesiredSize(rd, size))
	{
		rd.area = ApplyFill(size, rd.area);
		if(fillMode == FillMode::Fill)
			size = rd.area.size;
	}

	// Apply alignment
	rd.area = ApplyAlignment(Rect(rd.area.pos, size), scissorRect);

#if _DEBUG
	rd.guiRenderer->ClearScissorRect();
	rd.guiRenderer->RenderRect(*rd.rq, rd.area, Color::Magenta.WithAlpha(0.1f));
#endif
	element->Render(rd);
}
bool BoxSlotBase::GetDesiredSize(GUIRenderData rd, Vector2& sizeOut)
{
	if(!element->GetDesiredSize(rd, sizeOut))
		return false;

	if(rd.isSizeFixed)
	{
		if(fillMode != FillMode::None)
			sizeOut = ApplyFill(FillMode::Fit, sizeOut, rd.area).size;
	}

	return true;
}
Rect BoxSlotBase::ApplyAlignment(const Rect& rect, const Rect& parent)
{
	Vector2 remaining = parent.size - rect.size;
	remaining.x = Math::Max<float>(0, remaining.x);
	remaining.y = Math::Max<float>(0, remaining.y);

	return Rect(rect.pos + remaining * alignment, rect.size);
}