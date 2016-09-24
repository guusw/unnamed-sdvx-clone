#include "stdafx.h"
#include "GUI.hpp"
#include "GUIRenderer.hpp"

// Vertical/Horizontal Boxes

LayoutBox::~LayoutBox()
{
	for(auto s : m_children)
	{
		delete s;
	}
}

void LayoutBox::PreRender(GUIRenderData rd, GUIElementBase*& inputElement)
{
	m_TickAnimations(rd.deltaTime);

	Rect sourceRect = rd.area;
	Vector<float> elementSizes = CalculateSizes(rd);

	float offset = 0.0f;
	for(size_t i = 0; i < m_children.size(); i++)
	{
		float mySize = elementSizes[i];

		rd.area = sourceRect;
		if(layoutDirection == Vertical)
		{
			rd.area.pos.y += offset;
			rd.area.size.y = mySize;
		}
		else
		{
			rd.area.pos.x += offset;
			rd.area.size.x = mySize;
		}

		m_children[i]->PreRender(rd, inputElement);
		offset += mySize;
	}
}

void LayoutBox::Render(GUIRenderData rd)
{
	if(visibility != Visibility::Visible)
		return;

	for(size_t i = 0; i < m_children.size(); i++)
	{
		m_children[i]->Render(rd);
	}
}
Vector2 LayoutBox::GetDesiredSize(GUIRenderData rd)
{
	if(visibility == Visibility::Collapsed)
		return Vector2();

	Vector2 sizeOut;
	for(auto it = m_children.begin(); it != m_children.end(); it++)
	{
		Vector2 elemSize = (*it)->GetDesiredSize(rd);
		if(layoutDirection == Vertical)
		{
			sizeOut.y += elemSize.y;
			sizeOut.x = Math::Max(sizeOut.x, elemSize.x);
		}
		else
		{
			sizeOut.x += elemSize.x;
			sizeOut.y = Math::Max(sizeOut.y, elemSize.y);
		}
	}
	return sizeOut;
}
LayoutBox::Slot* LayoutBox::Add(GUIElement element)
{
	bool found = false;
	for(auto it = m_children.begin(); it != m_children.end(); it++)
	{
		if((*it)->element == element)
		{
			return *it; // Already exists
		}
	}

	Slot* slot = CreateSlot<LayoutBox::Slot>(element);
	m_children.AddUnique(slot);
	return slot;
}
void LayoutBox::Remove(GUIElement element)
{
	for(auto it = m_children.begin(); it != m_children.end();)
	{
		if((*it)->element == element)
		{
			m_children.erase(it);
			break;
		}
		else
		{
			it++;
		}
	}
}

Vector<float> LayoutBox::CalculateSizes(const GUIRenderData& rd) const
{
	// Combined size of everything
	float minSize = 0.0f;
	float fixedSize = 0.0f;
	float fillCount = 0;
	for(auto it = m_children.begin(); it != m_children.end(); it++)
	{
		Vector2 size = (*it)->GetDesiredSize(rd);
		float currentSize = (layoutDirection == Horizontal) ? size.x : size.y;
		bool currentFill = (layoutDirection == Horizontal) ? (*it)->fillX : (*it)->fillY;
		minSize += currentSize;
		if(currentFill)
		{
			fillCount += (*it)->fillAmount;
		}
		else
		{
			fixedSize += currentSize;
		}
	}

	float maxSize = (layoutDirection == Horizontal) ? rd.area.size.x : rd.area.size.y;

	// Down scale all fixed elements when their size exceeds the allocated area
	// TODO: Maybe allow clipping with additional flag instead of scaling down
	float fillSpace = Math::Max(0.0f, maxSize - fixedSize);
	float fixedScale = 1.0f;
	if(fixedSize > maxSize) 
	{
		assert(fillSpace == 0.0f);
		fixedScale = maxSize / fixedSize;
		fixedSize *= fixedScale;
	}

	float offset = 0.0f;
	Vector<float> ret;
	for(auto it = m_children.begin(); it != m_children.end(); it++)
	{
		Vector2 size = (*it)->GetDesiredSize(rd);
		float currentSize = (layoutDirection == Horizontal) ? size.x : size.y;
		bool currentFill = (layoutDirection == Horizontal) ? (*it)->fillX : (*it)->fillY;
		float mySize;
		if(currentFill)
		{
			float fillMult = (*it)->fillAmount / fillCount;
			mySize = fillSpace * fillMult;
		}
		else
		{
			mySize = currentSize * fixedScale;
		}
		ret.Add(mySize);
	}

	return ret;
}

const Vector<LayoutBox::Slot*>& LayoutBox::GetChildren()
{
	return m_children;
}
void LayoutBox::Clear()
{
	for(auto s : m_children)
	{
		delete s;
	}
	m_children.clear();
}

void LayoutBox::Slot::PreRender(GUIRenderData rd, GUIElementBase*& inputElement)
{
	Vector2 size = GetDesiredSize(rd);

	// Padding
	rd.area = padding.Apply(rd.area);

	// Filling
	if(!fillX || !fillY)
	{
		Rect rect = rd.area;
		if(!fillX && size.x < rd.area.size.x)
		{
			rect.size.x = size.x;
		}
		if(!fillY && size.y < rd.area.size.y)
		{
			rect.size.y = size.y;
		}
		rd.area = GUISlotBase::ApplyAlignment(alignment, rect, rd.area);
	}
	m_cachedArea = rd.area;

	element->PreRender(rd, inputElement);
}
void LayoutBox::Slot::Render(GUIRenderData rd)
{
	rd.area = m_cachedArea;
	if(!allowOverflow)
		rd.guiRenderer->PushScissorRect(rd.area);
	element->Render(rd);
	if(!allowOverflow)
		rd.guiRenderer->PopScissorRect();
}
