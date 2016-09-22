#include "stdafx.h"
#include "Spinner.hpp"
#include "GUIRenderer.hpp"

Spinner::Spinner(Ref<CommonGUIStyle> style)
{
	m_style = style;
}
void Spinner::Render(GUIRenderData rd)
{
	Rect rect = rd.area;
	m_rotation += rd.deltaTime * 640.0f;

	Transform transform;
	transform *= Transform::Translation(rect.pos + rect.size * 0.5f);
	transform *= Transform::Scale(Vector3(rect.size.x, rect.size.y, 1.0f));
	transform *= Transform::Rotation(Vector3(0.0f, 0.0f, m_rotation));
	transform *= Transform::Translation({ -0.5f, -0.5f, 0.0f });
	//transform *= Transform::Translation(rect.size * 0.5f);
	MaterialParameterSet params;
	params.SetParameter("color", Color::White);
	params.SetParameter("mainTex", m_style->spinnerTexture);
	rd.rq->Draw(transform, rd.guiRenderer->guiQuad, rd.guiRenderer->textureMaterial, params);
	
}
Vector2 Spinner::GetDesiredSize(GUIRenderData rd)
{
	return m_style->spinnerTexture->GetSize();
}
