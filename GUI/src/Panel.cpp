#include "stdafx.h"
#include "GUI.hpp"
#include "GUIRenderer.hpp"


// Panel(Image,Color) container

Panel::~Panel()
{
	if(m_content)
		delete m_content;
}

void Panel::PreRender(GUIRenderData rd, GUIElementBase*& inputElement)
{
	m_TickAnimations(rd.deltaTime);

	if(m_content)
	{
		m_content->PreRender(rd, inputElement);
	}
}

void Panel::Render(GUIRenderData rd)
{
	if(visibility != Visibility::Visible)
		return;

	Rect imageSize = rd.area;
	if(texture)
	{
		imageSize = GUISlotBase::ApplyFill(imageFillMode, texture->GetSize(), rd.area);
		imageSize = GUISlotBase::ApplyAlignment(imageAlignment, imageSize, rd.area);
	}
	rd.guiRenderer->RenderRect(imageSize, color, texture);

	if(m_content)
	{
		m_content->Render(rd);
	}
}
Vector2 Panel::GetDesiredSize(GUIRenderData rd)
{
	if(visibility == Visibility::Collapsed)
		return Vector2();

	if(!texture)
		return Vector2();

	Vector2 sizeOut = texture->GetSize();
	if(imageFillMode == FillMode::Fill || imageFillMode == FillMode::Fit)
	{
		Rect fill = GUISlotBase::ApplyFill(imageFillMode, sizeOut, rd.area);
		sizeOut = fill.size;
	}

	return sizeOut;
}

Panel::Slot* Panel::SetContent(GUIElement content)
{
	if(m_content)
	{
		delete m_content;
		m_content = nullptr;
	}
	if(content)
	{
		m_content = CreateSlot<Slot>(content);
	}
	return m_content;
}
Panel::Slot* Panel::GetContentSlot()
{
	return m_content;
}

void Panel::Slot::PreRender(GUIRenderData rd, GUIElementBase*& inputElement)
{
	rd.area = padding.Apply(rd.area);

	Vector2 size = GetDesiredSize(rd);

	rd.area = GUISlotBase::ApplyAlignment(alignment, Rect(Vector2(),size), rd.area);
	m_cachedArea = rd.area;

	element->PreRender(rd, inputElement);
}
void Panel::Slot::Render(GUIRenderData rd)
{
	rd.area = m_cachedArea;
	element->Render(rd);
}
Vector2 Panel::Slot::GetDesiredSize(GUIRenderData rd)
{
	return element->GetDesiredSize(rd);
}
