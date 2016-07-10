#include "stdafx.h"
#include "GUIRenderer.hpp"
#include "GUIRenderData.hpp"
#include "GUI.hpp"
#include "Application.hpp"

bool GUIRenderer::Init(class OpenGL* gl)
{
	ClearScissorRect();

	assert(gl);
	m_gl = gl;

	// Font
	CheckedLoad(font = FontRes::Create(g_gl, "fonts/segoeui.ttf"));

	// Load GUI shaders
	CheckedLoad(fontMaterial = g_application->LoadMaterial("font"));
	fontMaterial->opaque = false;
	CheckedLoad(textureMaterial = g_application->LoadMaterial("guiTex"));
	textureMaterial->opaque = false;
	CheckedLoad(colorMaterial = g_application->LoadMaterial("guiColor"));
	colorMaterial->opaque = false;

	guiQuad = MeshGenerators::Quad(g_gl, Vector2(0, 0), Vector2(1, 1));

	return true;
}
void GUIRenderer::Render(float deltaTime, Rect viewportSize, Ref<class GUIElementBase> rootElement)
{
	// Render state/queue for the GUI
	RenderState guiRs = g_application->GetRenderStateBase();
	guiRs.projectionTransform = g_application->GetGUIProjection();
	m_renderQueue = new RenderQueue(g_gl, guiRs);

	// Render GUI
	GUIRenderData grd;
	grd.rq = m_renderQueue;
	grd.guiRenderer = this;
	grd.deltaTime = deltaTime;
	grd.area = Rect(Vector2(), g_gameWindow->GetWindowSize());
	grd.transform = Transform();

	rootElement->Render(grd);
	/// NOTE: GUI is the other way around
	glCullFace(GL_FRONT);

	glClearColor(0.0f, 0.0f, 0.0f, 0);
	glClear(GL_COLOR_BUFFER_BIT);
	m_renderQueue->Process();

	delete m_renderQueue;
	m_renderQueue = nullptr;
}

void GUIRenderer::SetScissorRect(const Rect& scissor)
{
	m_scissorRect = scissor;
}
void GUIRenderer::ClearScissorRect()
{
	m_scissorRect = Rect(Vector2(), Vector2(-1));
}

Vector2i GUIRenderer::GetTextSize(const WString& str, uint32 fontSize /*= 16*/)
{
	Text text = font->CreateText(str, fontSize);
	return text->size;
}
Vector2i GUIRenderer::GetTextSize(const String& str, uint32 fontSize /*= 16*/)
{
	return GetTextSize(Utility::ConvertToUnicode(str), fontSize);
}
Vector2i GUIRenderer::RenderText(RenderQueue& rq, const WString& str, const Vector2& position, const Color& color /*= Color(1.0f)*/, uint32 fontSize /*= 16*/)
{
	Text text = font->CreateText(str, fontSize);
	Transform textTransform;
	textTransform *= Transform::Translation(position);
	MaterialParameterSet params;
	params.SetParameter("color", color);
	rq.DrawScissored(m_scissorRect, textTransform, text, fontMaterial, params);
	return text->size;
}
Vector2i GUIRenderer::RenderText(RenderQueue& rq, const String& str, const Vector2& position, const Color& color /*= Color(1.0f)*/, uint32 fontSize /*= 16*/)
{
	return RenderText(rq, Utility::ConvertToUnicode(str), position, color, fontSize);
}
void GUIRenderer::RenderText(RenderQueue& rq, Text& text, const Vector2& position, const Color& color /*= Color(1.0f)*/)
{
	Transform textTransform;
	textTransform *= Transform::Translation(position);
	MaterialParameterSet params;
	params.SetParameter("color", color);
	rq.DrawScissored(m_scissorRect, textTransform, text, fontMaterial, params);
}

void GUIRenderer::RenderRect(RenderQueue& rq, const Rect& rect, const Color& color /*= Color(1.0f)*/, Texture texture /*= Texture()*/)
{
	Transform transform;
	transform *= Transform::Translation(rect.pos);
	transform *= Transform::Scale(Vector3(rect.size.x, rect.size.y, 1.0f));
	MaterialParameterSet params;
	params.SetParameter("color", color);
	if(texture)
	{
		params.SetParameter("mainTex", texture);
		rq.DrawScissored(m_scissorRect, transform, guiQuad, textureMaterial, params);
	}
	else
	{
		rq.DrawScissored(m_scissorRect, transform, guiQuad, colorMaterial, params);
	}
}
