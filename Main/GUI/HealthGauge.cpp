#include "stdafx.h"
#include "GUIRenderer.hpp"
#include "HealthGauge.hpp"
#include "Application.hpp"

HealthGauge::HealthGauge()
{
	barMargin = Margin(20, 38, 20, 38);
}

void HealthGauge::Render(GUIRenderData rd)
{
	// Calculate bar placement to fit parent rectangle
	Rect barArea = GUISlotBase::ApplyFill(FillMode::Fit, frameTexture->GetSize(), rd.area);
	float elementScale = barArea.size.x / frameTexture->GetSize().x; // Scale of the original bar
	GUISlotBase::ApplyAlignment(Vector2(0.5f), barArea, rd.area);

	// Optional Bg?
	if(bgTexture)
		rd.guiRenderer->RenderRect(barArea, Color::White, bgTexture);

	// Acquire bar inner area
	Margin scaledMargin = barMargin * elementScale;
	Rect fillArea = scaledMargin.Apply(barArea);

	// Draw the fill for the gauge
	Transform transform;
	transform *= Transform::Translation(fillArea.pos);
	transform *= Transform::Scale(Vector3(fillArea.size.x, fillArea.size.y, 1.0f));
	MaterialParameterSet params;
	params.SetParameter("mainTex", fillTexture);
	params.SetParameter("rate", rate);
	Color color;
	if(rate > 0.75f)
	{
		// Fade to max
		float f2 = (rate - 0.75f) / 0.25f;
		f2 = powf(f2, 1.2f);
		color = Colori(90, 225, 58);
		color *= (0.75f + f2 * 0.25f);
	}
	else
	{
		float f1 = (rate / 0.75f);
		color = Colori(146, 109, 141);
		color *= (0.75f + f1 * 0.25f);
	}
	params.SetParameter("barColor", color);
	rd.rq->Draw(transform, rd.guiRenderer->guiQuad, fillMaterial, params);

	// Draw frame last
	rd.guiRenderer->RenderRect(barArea, Color::White, frameTexture);
}

Vector2 HealthGauge::GetDesiredSize(GUIRenderData rd)
{
	return frameTexture->GetSize();
}
