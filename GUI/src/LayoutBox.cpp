#include "stdafx.h"
#include "GUI.hpp"
#include "GUIRenderer.hpp"
#include "LayoutBox.hpp"

// Vertical/Horizontal Boxes

LayoutBox::~LayoutBox()
{
	for(auto s : m_children)
	{
		delete s;
	}
}
void LayoutBox::UpdateAnimations(float deltaTime)
{
	GUIElementBase::UpdateAnimations(deltaTime);
	for(auto c : m_children)
	{
		c->element->UpdateAnimations(deltaTime);
	}
}
void LayoutBox::Update(GUIUpdateData data)
{
	if(visibility != Visibility::Visible)
		return;

	m_UpdateTransform(data);

	if(!m_composition.IsValid())
	{
		m_composition.Update();
		Rect sourceRect = data.area;
		Vector<float> elementSizes = CalculateSizes(data);

		float offset = 0.0f;
		for(size_t i = 0; i < m_children.size(); i++)
		{
			float mySize = elementSizes[i];

			data.area = sourceRect;
			if(layoutDirection == Vertical)
			{
				data.area.pos.y += offset;
				data.area.size.y = mySize;
			}
			else
			{
				data.area.pos.x += offset;
				data.area.size.x = mySize;
			}

			m_children[i]->Update(data);
			offset += mySize;
		}
	}
	else
	{
		for(auto c : m_children)
		{
			c->Update(data);
		}
	}
}
void LayoutBox::Render(GUIRenderData data)
{
	if(visibility != Visibility::Visible)
		return;

	for(size_t i = 0; i < m_children.size(); i++)
	{
		m_children[i]->Render(data);
	}

	// Render Debug
	if(data.debug)
	{
		data.guiRenderer->RenderWireBox(m_cachedArea.ToTransform(), Color(0.2f, 0.2f, 1.0f, 0.5f));
	}
}
Vector2 LayoutBox::m_GetDesiredBaseSize(GUIUpdateData data)
{
	Vector2 sizeOut;
	for(auto it = m_children.begin(); it != m_children.end(); it++)
	{
		Vector2 elemSize = (*it)->GetDesiredSize(data);
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

Vector<float> LayoutBox::CalculateSizes(const GUIUpdateData& data) const
{
	// Combined size of everything
	float minSize = 0.0f;
	float fixedSize = 0.0f;
	float fillSize = 0.0f;
	float fillCount = 0;
	for(auto it = m_children.begin(); it != m_children.end(); it++)
	{
		Vector2 size = (*it)->GetDesiredSize(data);
		float currentSize = (layoutDirection == Horizontal) ? size.x : size.y;
		bool currentFill = (layoutDirection == Horizontal) ? (*it)->fillX : (*it)->fillY;
		minSize += currentSize;
		if(currentFill)
		{
			fillCount += (*it)->fillAmount;
			fillSize += currentSize;
		}
		else
		{
			fixedSize += currentSize;
		}
	}

	float maxSize = (layoutDirection == Horizontal) ? data.area.size.x : data.area.size.y;

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
	float fillScale = fillSpace / fillSize;
	fillSize *= fillScale;


	float offset = 0.0f;
	Vector<float> ret;
	for(auto it = m_children.begin(); it != m_children.end(); it++)
	{
		Vector2 size = (*it)->GetDesiredSize(data);
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

void LayoutBox::m_PropagateEventToChildren(GUIEvent& event)
{
	for(auto s : m_children)
	{
		event.Propagate(*s->element);
	}
}
void LayoutBox::m_InvalidateSlotAreas()
{
	for(auto c : m_children)
	{
		c->InvalidateArea();
	}
}
void LayoutBox::m_OnChildSlotChanged(GUISlotBase* slot)
{
	if(!m_alreadyInvalidatingChildren)
	{
		m_alreadyInvalidatingChildren = true;

		m_composition.Invalidate();
		InvalidationEvent invalidateChildren;
		for(auto c : m_children)
		{
			// Invalidate child slots when composition is invalidated
			c->InvalidateArea();
			invalidateChildren.Propagate(*c->element);
		}

		m_alreadyInvalidatingChildren = false;
	}
}
void LayoutBox::Clear()
{
	for(auto s : m_children)
	{
		delete s;
	}
	m_children.clear();
}

LayoutBox::Slot::Slot()
{
	auto invalidateArea = &GUISlotBase::InvalidateArea;
	fillX.Notify.Bind(this, invalidateArea);
	fillY.Notify.Bind(this, invalidateArea);
	fillAmount.Notify.Bind(this, invalidateArea);
}
void LayoutBox::Slot::Update(GUIUpdateData data)
{
	if(!m_cachedArea.IsValid())
	{
		Vector2 size = GetDesiredSize(data);

		// Padding
		data.area = padding->Apply(data.area);

		// Filling
		if(!fillX || !fillY)
		{
			Rect rect = data.area;
			if(!fillX && size.x < data.area.size.x)
			{
				rect.size.x = size.x;
			}
			if(!fillY && size.y < data.area.size.y)
			{
				rect.size.y = size.y;
			}
			data.area = GUISlotBase::ApplyAlignment(alignment, rect, data.area);
		}
		m_UpdateArea(data.area);
	}

	data.area = m_cachedArea;
	element->Update(data);
}
void LayoutBox::Slot::Render(GUIRenderData rd)
{
	if(!allowOverflow)
		rd.guiRenderer->PushScissorRect(m_cachedArea);
	element->Render(rd);
	if(!allowOverflow)
		rd.guiRenderer->PopScissorRect();

	// Render Debug
	if(rd.debug)
	{
		Rect debug = m_cachedArea;
		debug.Offset(-1.0f);
		rd.guiRenderer->RenderWireBox(debug.ToTransform(), Color::Blue.WithAlpha(0.5f));
	}
}
