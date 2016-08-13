#include "stdafx.h"
#include "GUIRenderer.hpp"
#include "HealthGauge.hpp"
#include "Application.hpp"

HealthGauge::HealthGauge()
{
	m_fillMaterial = g_application->LoadMaterial("gauge");
	m_fillMaterial->opaque = false;

	m_bg = g_application->LoadTexture("gauge_bg.png");
	m_frame = g_application->LoadTexture("gauge_base_t.png");
	m_fill = g_application->LoadTexture("gauge_fill.png");
}

void HealthGauge::Render(GUIRenderData rd)
{
	// Calculate bar placement to fit parent rectangle
	Rect barArea = GUISlotBase::ApplyFill(FillMode::Fit, m_frame->GetSize(), rd.area);
	float elementScale = barArea.size.x / m_frame->GetSize().x; // Scale of the original bar
	GUISlotBase::ApplyAlignment(Vector2(0.5f), barArea, rd.area);

	rd.guiRenderer->RenderRect(barArea, Color::White, m_bg);

	// Acquire bar inner area
	Margin barMargin = Margin(20, 38, 20, 38) * elementScale;
	Rect fillArea = barMargin.Apply(barArea);

	// Draw the fill for the gauge
	Transform transform;
	transform *= Transform::Translation(fillArea.pos);
	transform *= Transform::Scale(Vector3(fillArea.size.x, fillArea.size.y, 1.0f));
	MaterialParameterSet params;
	params.SetParameter("mainTex", m_fill);
	params.SetParameter("rate", rate);
	params.SetParameter("barColor", Colori(255, 0, 114));
	rd.rq->Draw(transform, rd.guiRenderer->guiQuad, m_fillMaterial, params);

	// Draw frame last
	rd.guiRenderer->RenderRect(barArea, Color::White, m_frame);
}

Vector2 HealthGauge::GetDesiredSize(GUIRenderData rd)
{
	return m_frame->GetSize();
}
