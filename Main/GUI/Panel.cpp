#include "stdafx.h"
#include "GUI.hpp"
#include "GUIRenderer.hpp"


// Panel(Image,Color) container

Panel::~Panel()
{
	if(m_content)
		delete m_content;
}
void Panel::Render(GUIRenderData rd)
{
	Rect imageSize = rd.area;
	if(texture)
	{
		imageSize = GUISlotBase::ApplyFill(imageFillMode, texture->GetSize(), rd.area);
	}
	rd.guiRenderer->RenderRect(*rd.rq, imageSize, color, texture);

	if(m_content)
	{
		m_content->Render(rd);
	}
}
bool Panel::GetDesiredSize(GUIRenderData rd, Vector2& sizeOut)
{
	if(!texture)
		return false;

	sizeOut = texture->GetSize();
	return true;
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

void Panel::Slot::Render(GUIRenderData rd)
{
	Vector2 size;
	if(GetDesiredSize(rd, size))
	{
		 Rect newRect = GUISlotBase::ApplyAlignment(alignment, Rect(Vector2(),size), rd.area);
		 rd.area = newRect;
	}

	element->Render(rd);
}
