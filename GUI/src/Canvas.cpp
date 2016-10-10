#include "stdafx.h"
#include "GUI.hpp"
#include "GUIRenderer.hpp"

// Canvas

Canvas::~Canvas()
{
	for(auto& c : m_children)
	{
		delete c;
	}
}

void Canvas::PreRender(GUIRenderData rd, GUIElementBase*& inputElement)
{
	for(auto& c : m_children)
	{
		c->PreRender(rd, inputElement);
	}
}
void Canvas::Render(GUIRenderData rd)
{
	m_TickAnimations(rd.deltaTime);

	if(visibility != Visibility::Visible)
		return;

	for(auto& c : m_children)
	{
		c->Render(rd);
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
	m_SortChildren();
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

	for(auto& c : m_children)
	{
		delete c;
	}
	m_children.clear();
}
const Vector<Canvas::Slot*>& Canvas::GetChildren()
{
	return m_children;
}
void Canvas::m_OnZOrderChanged(GUISlotBase* slot)
{
	m_SortChildren();
}
void Canvas::m_SortChildren()
{
	m_children.Sort([](const GUISlotBase* l, const GUISlotBase* r)
	{
		return l->GetZOrder() < r->GetZOrder();
	});
}
Vector2 Canvas::GetDesiredSize(GUIRenderData rd)
{
	Vector2 sizeOut = Vector2(0, 0);
	for(auto s : m_children)
	{
		Vector2 elemSize = s->GetDesiredSize(rd);
		sizeOut.x = Math::Max(sizeOut.x, elemSize.x);
		sizeOut.y = Math::Max(sizeOut.y, elemSize.y);
	}
	return sizeOut;
}

void Canvas::Slot::PreRender(GUIRenderData rd, GUIElementBase*& inputElement)
{
	// Apply anchor and offset to get the canvas rectangle
	rd.area = anchor.Apply(rd.area);
	
	// Fixed size mode
	if(size != Vector2::Zero)
	{
		rd.area = ApplyAlignment(alignment, Rect(Vector2(), size), rd.area);
	}
	else
	{
		// Perform auto-sizing
		{
			Vector2 desiredSize = GetDesiredSize(rd) + offset.size;
			Rect autoSized = ApplyAlignment(alignment, Rect(Vector2(), desiredSize), rd.area);
			if(autoSizeX)
			{
				rd.area.pos.x = autoSized.pos.x;
				rd.area.size.x = autoSized.size.x;
			}
			if(autoSizeY)
			{
				rd.area.pos.y = autoSized.pos.y;
				rd.area.size.y = autoSized.size.y;
			}
		}
	}

	rd.area.size += offset.size;
	rd.area.pos += offset.pos;

	// Apply padding
	rd.area = padding.Apply(rd.area);
	// Cache area
	m_cachedArea = rd.area;
	
	element->PreRender(rd, inputElement);

}
void Canvas::Slot::Render(GUIRenderData rd)
{
	rd.area = m_cachedArea;
	if(!allowOverflow)
		rd.guiRenderer->PushScissorRect(m_cachedArea);
	element->Render(rd);
	if(!allowOverflow)
		rd.guiRenderer->PopScissorRect();
}
void Canvas::Slot::AutoSize(bool enabled)
{
	autoSizeX = enabled;
	autoSizeY = enabled;
}
