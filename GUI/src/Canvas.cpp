#include "stdafx.h"
#include "GUI.hpp"
#include "GUIRenderer.hpp"
#include <Shared/Color.hpp>

// Canvas
Canvas::~Canvas()
{
	for(auto c : m_children)
	{
		delete c;
	}
}
void Canvas::UpdateAnimations(float deltaTime)
{
	GUIElementBase::UpdateAnimations(deltaTime);
	for(auto c : m_children)
	{
		c->element->UpdateAnimations(deltaTime);
	}
}
void Canvas::Update(GUIUpdateData data)
{
	if(visibility != Visibility::Visible)
		return;

	m_UpdateTransform(data);

	for(auto c : m_children)
	{
		c->Update(data);
	}
}
void Canvas::Render(GUIRenderData data)
{
	if(visibility != Visibility::Visible)
		return;

	// Update sorting order?
	//	this should only have a visual effect, move to update if needed
	if(!m_sortingOrder.IsValid())
	{
		m_SortChildren();
		m_sortingOrder.Update();
	}

	for(auto& c : m_children)
	{
		c->element->Render(data);
	}
}

class Canvas::Slot* Canvas::Add(GUIElement element)
{
	bool found = false;
	for(auto it = m_children.begin(); it != m_children.end(); it++)
	{
		if((*it)->element == element)
			return (*it); // Already exists
	}

	Slot* slot = CreateSlot<Canvas::Slot>(element);
	m_children.AddUnique(slot);
	m_sortingOrder.Invalidate();

	return slot;
}
void Canvas::Remove(GUIElement element)
{
	for(auto it = m_children.begin(); it != m_children.end();)
	{
		if((*it)->element == element)
		{
			delete *it;
			it = m_children.erase(it);
			return;
		}
		else
			it++;
	}
}
void Canvas::Clear()
{
	for(auto c : m_children)
	{
		delete c;
	}
	m_children.clear();
}
const Vector<Canvas::Slot*>& Canvas::GetChildren()
{
	return m_children;
}
GUIElementBase* Canvas::SelectNext(GUIElementBase* from, GUIElementBase* item, int dir, int layoutDirection)
{
	// TODO: Implement canvas navigation
	return nullptr;
}
void Canvas::m_OnChildSlotChanged(GUISlotBase* slot)
{
}
void Canvas::m_OnChildZOrderChanged(GUISlotBase* slot)
{
	m_sortingOrder.Invalidate();
}
void Canvas::m_PropagateEventToChildren(GUIEvent& event)
{
	for(auto c : m_children)
	{
		event.Propagate(*c->element);
	}
}
void Canvas::m_InvalidateSlotAreas()
{
	for(auto c : m_children)
	{
		c->InvalidateArea();
	}
}
void Canvas::m_SortChildren()
{
	m_children.Sort([](const GUISlotBase* l, const GUISlotBase* r)
	{
		return l->zOrder < r->zOrder;
	});
}
Vector2 Canvas::m_GetDesiredBaseSize(GUIUpdateData data)
{
	Vector2 sizeOut = Vector2(0, 0);
	for(auto s : m_children)
	{
		Vector2 elemSize = s->GetDesiredSize(data);
		sizeOut.x = Math::Max(sizeOut.x, elemSize.x);
		sizeOut.y = Math::Max(sizeOut.y, elemSize.y);
	}
	return sizeOut;
}

Canvas::Slot::Slot()
{
	auto invalidateArea = &GUISlotBase::InvalidateArea;

	// Area changes
	offset.Notify.Bind(this, invalidateArea);
	anchor.Notify.Bind(this, invalidateArea);

	// Default fill
	fillX = true;
	fillY = true;
}
void Canvas::Slot::Update(GUIUpdateData data)
{
	if(!m_cachedArea.IsValid())
	{
		data.area = anchor->Apply(data.area);

		data.area.pos += offset->pos;
		data.area.size += offset->size;

		Vector2 childSize;
		if(!fillX || !fillY)
			childSize = element->GetDesiredSize(data);

		if(fillX)
			childSize.x = data.area.size.x;
		if(fillY)
			childSize.y = data.area.size.y;

		Rect childRect = Rect(Vector2(), childSize);
		data.area = ApplyAlignment(alignment, childRect, data.area);
		data.area = padding->Apply(data.area);
		m_UpdateArea(data.area);
	}
	data.area = m_cachedArea;

	element->Update(data);
}