#include "stdafx.h"
#include "GUI.hpp"
#include "GUIRenderer.hpp"

// Panel(Image,Color) container

void Panel::Render(GUIRenderData rd)
{
	rd.guiRenderer->RenderRect(*rd.rq, rd.area, color, texture);

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