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

	// Offset in frame where the fill is drawn to
	Margin barMargin;

	Material fillMaterial;
	Texture frameTexture;
	Texture fillTexture;
	Texture bgTexture;
};