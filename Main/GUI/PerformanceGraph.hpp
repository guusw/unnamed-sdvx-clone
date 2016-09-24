#pragma once
#include <GUI/LayoutBox.hpp>

class PerformanceGraph : public GUIElementBase
{
public:
	PerformanceGraph();
	virtual void PreRender(GUIRenderData rd, GUIElementBase*& inputElement);
	virtual void Render(GUIRenderData rd);
	virtual Vector2 GetDesiredSize(GUIRenderData rd);

	Texture borderTexture;
	Margini border;
};