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
void LayoutBox::Render(GUIRenderData rd)
{
	m_TickAnimations(rd.deltaTime);

	if(visibility != Visibility::Visible)
		return;

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

		m_children[i]->Render(rd);
		offset += mySize;
	}
}
bool LayoutBox::GetDesiredSize(GUIRenderData rd, Vector2& sizeOut)
{
	if(visibility == Visibility::Collapsed)
		return false;

	sizeOut = Vector2(0, 0);
	bool set = false;
	for(auto it = m_children.begin(); it != m_children.end(); it++)
	{
		Vector2 elemSize;
		if((*it)->GetDesiredSize(rd,elemSize))
		{
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
			set = true;
		}
	}
	return set;
}
LayoutBox::Slot* LayoutBox::Add(GUIElement element)
{
	bool found = false;
	for(auto it = m_children.begin(); it != m_children.end(); it++)
	{
		if((*it)->element == element)
			return *it; // Already exists
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
			m_children.erase(it);
		else
			it++;
	}
}

Vector<float> LayoutBox::CalculateSizes(const GUIRenderData& rd) const
{
	// Combined size of everything
	float minSize = 0.0f;
	float fixedSize = 0.0f;
	uint32 fillCount = 0;
	for(auto it = m_children.begin(); it != m_children.end(); it++)
	{
		Vector2 size;
		(*it)->GetDesiredSize(rd, size);
		float currentSize = (layoutDirection == Horizontal) ? size.x : size.y;
		minSize += currentSize;
		if((*it)->fill)
		{
			fillCount++;
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

	// Reciprocal of fill count
	float fillMult = 1.0f / fillCount;

	float offset = 0.0f;
	Vector<float> ret;
	for(auto it = m_children.begin(); it != m_children.end(); it++)
	{
		Vector2 size;
		(*it)->GetDesiredSize(rd, size);
		float currentSize = (layoutDirection == Horizontal) ? size.x : size.y;
		float mySize;
		if((*it)->fill)
		{
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
