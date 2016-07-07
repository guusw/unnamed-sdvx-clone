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
void Canvas::Render(GUIRenderData rd)
{
	for(auto& c : m_children)
	{
		c->Render(rd);
	}
}
void Canvas::Add(GUIElement element)
{
	bool found = false;
	for(auto it = m_children.begin(); it != m_children.end(); it++)
	{
		if((*it)->element == element)
			return; // Already exists
	}

	m_children.AddUnique(CreateSlot<Canvas::Slot>(element));
	m_SortChildren();
}
void Canvas::Remove(GUIElement element)
{
	for(auto it = m_children.begin(); it != m_children.end();)
	{
		if((*it)->element == element)
			m_children.erase(it);
		else
			it++;
	}
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

void Canvas::Slot::Render(GUIRenderData rd)
{
	rd.isSizeFixed = !autoSize;

	Vector2 usedSize = size;
	if(autoSize)
	{
		bool gotSize = GetDesiredSize(rd, usedSize);
	}

	Vector2 alignmentOffset = -alignment * usedSize;
	Vector2 anchorOffset = anchor * rd.area.size;
	Rect targetRect = Rect(rd.area.pos + pos + alignmentOffset + anchorOffset, usedSize);
	
	// Apply padding
	Rect scissorRect = rd.area = padding.Apply(targetRect);

	// Scissor Rectangle
	rd.guiRenderer->SetScissorRect(scissorRect);

	// Apply filling
	Vector2 size;
	if(GetDesiredSize(rd, size))
	{
		FillMode actualFillMode = fillMode;
		if(actualFillMode == FillMode::None)
			actualFillMode = FillMode::Stretch;
		rd.area = ApplyFill(actualFillMode, size, rd.area);
	}

#if _DEBUG
	rd.guiRenderer->ClearScissorRect();
	rd.guiRenderer->RenderRect(*rd.rq, rd.area, Color::Magenta.WithAlpha(0.1f));
#endif

	element->Render(rd);
}
