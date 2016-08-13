#pragma once
#include "GUIElement.hpp"

class HealthGauge : public GUIElementBase
{
public:
	HealthGauge();
	virtual void Render(GUIRenderData rd) override;
	virtual Vector2 GetDesiredSize(GUIRenderData rd) override;

	// The fill rate of the gauge
	float rate = 0.5f;

private:
	Material m_fillMaterial;
	Texture m_fill;
	Texture m_frame;
	Texture m_bg;
};