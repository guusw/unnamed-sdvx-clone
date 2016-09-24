#include "stdafx.h"
#include "PerformanceGraph.hpp"
#include <GUI/GUIRenderer.hpp>

PerformanceGraph::PerformanceGraph()
{

}
void PerformanceGraph::PreRender(GUIRenderData rd, GUIElementBase*& inputElement)
{
}
void PerformanceGraph::Render(GUIRenderData rd)
{
	rd.guiRenderer->RenderButton(rd.area, borderTexture, border);
	Rect inner = border.Apply(rd.area);

	/// TODO: Draw graph
}

Vector2 PerformanceGraph::GetDesiredSize(GUIRenderData rd)
{
	return rd.area.size;
}
