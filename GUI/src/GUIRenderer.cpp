#include "stdafx.h"
#include "GUIRenderer.hpp"
#include "GUIRenderData.hpp"
#include "GUI.hpp"
using namespace Graphics;

GUIRenderer::~GUIRenderer()
{
	assert(!m_renderQueue);
}
bool GUIRenderer::Init(class OpenGL* gl)
{
	assert(gl);
	m_gl = gl;

	m_time = 0.0f;

	// Font
	CheckedLoad(font = FontRes::Create(gl, "fonts/segoeui.ttf"));

	// Load GUI shaders
	CheckedLoad(fontMaterial = MaterialRes::Create(m_gl, "shaders/position_texture_wp.vs", "shaders/font.fs"));
	fontMaterial->opaque = false;
	CheckedLoad(textureMaterial = MaterialRes::Create(m_gl, "shaders/position_texture_wp_uvflip.vs", "shaders/texture_colored.fs"));
	textureMaterial->opaque = false;
	CheckedLoad(colorMaterial = MaterialRes::Create(m_gl, "shaders/position_texture_wp_uvflip.vs", "shaders/colored.fs"));
	colorMaterial->opaque = false;
	CheckedLoad(buttonMaterial = MaterialRes::Create(m_gl, "shaders/position_passthrough.vs", "shaders/gui_button.gs", "shaders/texture_colored.fs"));
	buttonMaterial->opaque = false;

	lineBox = MeshRes::Create(m_gl);
	Vector<MeshGenerators::SimpleVertex> lines = { 
		{ Vector3(0.0f, 0.0f, 0.0f), Vector2(0.0f, 0.0f) },
		{ Vector3(1.0f, 0.0f, 0.0f), Vector2(1.0f, 0.0f) },
		{ Vector3(1.0f, 1.0f, 0.0f), Vector2(0.0f, 1.0f) },
		{ Vector3(0.0f, 1.0f, 0.0f), Vector2(0.0f, 1.0f) },
		{ Vector3(0.0f, 0.0f, 0.0f), Vector2(0.0f, 0.0f) }, //Loop
	};
	lineBox->SetData(lines);
	lineBox->SetPrimitiveType(PrimitiveType::LineStrip);

	guiQuad = MeshGenerators::Quad(m_gl, Vector2(0, 0), Vector2(1, 1));

	pointMesh = MeshRes::Create(m_gl);
	Vector<MeshGenerators::SimpleVertex> points = { MeshGenerators::SimpleVertex(Vector3(0,0,0), Vector2(0,0)) };
	pointMesh->SetData(points);
	pointMesh->SetPrimitiveType(PrimitiveType::PointList);

	Vector<MeshGenerators::SimpleVertex> lineSegment = { 
		MeshGenerators::SimpleVertex(Vector3(0,0,0), Vector2(0,0)),
		MeshGenerators::SimpleVertex(Vector3(1,0,0), Vector2(1,0)) };
	lineMesh = MeshRes::Create(m_gl);
	lineMesh->SetData(lineSegment);
	lineMesh->SetPrimitiveType(PrimitiveType::LineStrip);

	return true;
}
Graphics::RenderQueue& GUIRenderer::Begin(const Rect& viewport)
{
	// Must have not called begin before this / or have called end
	assert(m_renderQueue == nullptr);

	// Set initial scissor rect to be disabled
	m_scissorRect = Rect(Vector2(0, 0), Vector2(-1));

	m_viewport = viewport;

	// Render state/queue for the GUI
	RenderState guiRs;
	guiRs.viewportSize = viewport.size;
	guiRs.projectionTransform = ProjectionMatrix::CreateOrthographic(0, viewport.size.x, viewport.size.y, 0.0f, -1.0f, 100.0f);
	guiRs.aspectRatio = viewport.size.y / viewport.size.x;
	guiRs.time = m_time;
	m_renderQueue = new RenderQueue(m_gl, guiRs);

	return *m_renderQueue;
}
void GUIRenderer::End()
{
	// Must have called Begin
	assert(m_renderQueue);

	// Render all elements placed in the queue previously

	Rect oldViewport = m_gl->GetViewport();
	m_gl->SetViewport(m_viewport);

	// NOTE: GUI is the other way around
	glCullFace(GL_FRONT);

	// Render everything in the queue
	m_renderQueue->Process();

	// Verify if scissor rectangle state was correctly restored
	assert(m_scissorRectangles.empty());

	delete m_renderQueue;
	m_renderQueue = nullptr;

	// Restore viewport
	m_gl->SetViewport(oldViewport);

	// Reset face culling mode
	glCullFace(GL_BACK);
}
void GUIRenderer::PushScissorRect(const Rect& scissor)
{
	if(!m_scissorRectangles.empty())
	{
		m_scissorRect = m_scissorRectangles.back().Clamp(scissor);
	}
	else
	{
		m_scissorRect = m_viewport.Clamp(scissor);
	}
	m_scissorRectangles.Add(m_scissorRect);
}
void GUIRenderer::PopScissorRect()
{
	m_scissorRectangles.pop_back();
	if(m_scissorRectangles.empty())
	{
		m_scissorRect = Rect(Vector2(), Vector2(-1));
	}
	else
	{
		m_scissorRect = m_scissorRectangles.back();
	}
}
Rect GUIRenderer::GetScissorRect() const
{
	if(m_scissorRectangles.empty())
	{
		return m_viewport;
	}
	else 
	{
		return m_scissorRectangles.back();
	}
}
void GUIRenderer::RenderText(const Transform2D& transform, Text text, const Color& color /*= Color::White*/)
{
	MaterialParameterSet params;
	params.SetParameter("color", color);
	m_renderQueue->Draw(transform, text, fontMaterial, params);
}
void GUIRenderer::RenderText(const Transform2D& transform, const Rect& scissorRectangle, Text text, const Color& color /*= Color::White*/)
{
	if(m_scissorRect.size.x == 0 || m_scissorRect.size.y == 0)
		return;

	MaterialParameterSet params;
	params.SetParameter("color", color);
	m_renderQueue->DrawScissored(m_scissorRect, transform, text, fontMaterial, params);
}
void GUIRenderer::RenderRect(const Transform2D& transform, const Color& color /*= Color::White*/, Texture texture /*= Texture()*/)
{
	MaterialParameterSet params;
	params.SetParameter("color", color);
	if(texture)
	{
		params.SetParameter("mainTex", texture);
		m_renderQueue->Draw(transform, guiQuad, textureMaterial, params);
	}
	else
	{
		m_renderQueue->Draw(transform, guiQuad, colorMaterial, params);
	}
}
void GUIRenderer::RenderRect(const Transform2D& transform, const Rect& scissorRectangle, const Color& color, Texture texture)
{
	if(m_scissorRect.size.x == 0 || m_scissorRect.size.y == 0)
		return;

	MaterialParameterSet params;
	params.SetParameter("color", color);
	if(texture)
	{
		params.SetParameter("mainTex", texture);
		m_renderQueue->DrawScissored(m_scissorRect, transform, guiQuad, textureMaterial, params);
	}
	else
	{
		m_renderQueue->DrawScissored(m_scissorRect, transform, guiQuad, colorMaterial, params);
	}
}
void GUIRenderer::RenderWireBox(const Transform2D& transform, const Color& color, float lineWidth)
{
	PointDrawCall* pdc = new PointDrawCall();
	pdc->size = lineWidth;
	pdc->worldTransform = transform; // 2D to 3D
	pdc->mesh = lineBox;
	pdc->mat = colorMaterial;
	pdc->params.SetParameter("color", color);
	m_renderQueue->Add(pdc);
}
void GUIRenderer::RenderPoint(const Transform2D& transform, const Color& color /*= Color::White*/, float pointSize /*= 1.0f*/)
{
	PointDrawCall* pdc = new PointDrawCall();
	pdc->size = pointSize;
	pdc->worldTransform = transform; // 2D to 3D
	pdc->mesh = pointMesh;
	pdc->mat = colorMaterial;
	pdc->params.SetParameter("color", color);
	m_renderQueue->Add(pdc);
}
void GUIRenderer::RenderLine(const Transform2D& transform, const Color& color /*= Color::White*/, float pointSize /*= 1.0f*/)
{
	PointDrawCall* pdc = new PointDrawCall();
	pdc->size = pointSize;
	pdc->worldTransform = transform; // 2D to 3D
	pdc->mesh = lineMesh;
	pdc->mat = colorMaterial;
	pdc->params.SetParameter("color", color);
	m_renderQueue->Add(pdc);
}
void GUIRenderer::RenderLine(const Vector2& a, const Vector2& b, const Color& color /*= Color::White*/, float pointSize /*= 1.0f*/)
{
	Vector2 d = (b - a);
	float l = d.Length();
	if(l < 0.1f)
		return;
	d /= l;
	float r = atan2(-d.y, d.x);
	//Transform2D t = Transform2D::Translation(a) * Transform2D::Rotation(r) * Transform2D::Scale(Vector2(l));
	Transform2D t = Transform2D::Translation(a) * Transform2D::Rotation(r * Math::radToDeg) * Transform2D::Scale(Vector2(l, 1.0f));
	RenderLine(t, color, pointSize);
}