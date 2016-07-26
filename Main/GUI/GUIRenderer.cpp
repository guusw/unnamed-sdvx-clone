#include "stdafx.h"
#include "GUIRenderer.hpp"
#include "GUIRenderData.hpp"
#include "GUI.hpp"
#include "Application.hpp"

GUIRenderer::~GUIRenderer()
{
	assert(!m_renderQueue);
	SetInputFocus(nullptr);
	SetWindow(nullptr);
}
bool GUIRenderer::Init(class OpenGL* gl, class Window* window)
{
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

	// Initial window assignment
	SetWindow(window);

	return true;
}
void GUIRenderer::Render(float deltaTime, Rect viewportSize, Ref<class GUIElementBase> rootElement)
{
	Begin();
	// Render GUI
	GUIRenderData grd;
	grd.rq = m_renderQueue;
	grd.guiRenderer = this;
	grd.deltaTime = deltaTime;
	grd.area = viewportSize;
	grd.transform = Transform();

	rootElement->Render(grd);

	// Clear text input after processing
	m_ResetTextInput();
	End();
}

Graphics::RenderQueue& GUIRenderer::Begin()
{
	// Must have not called begin before this / or have called end
	assert(m_renderQueue == nullptr);

	// Set initial scissor rect to be disabled
	m_scissorRect = Rect(Vector2(0, 0), Vector2(-1));

	// Render state/queue for the GUI
	RenderState guiRs = g_application->GetRenderStateBase();
	guiRs.projectionTransform = g_application->GetGUIProjection();
	m_renderQueue = new RenderQueue(g_gl, guiRs);

	return *m_renderQueue;
}
void GUIRenderer::End()
{
	// Must have called Begin
	assert(m_renderQueue);

	// Render all elements placed in the queue previously

	/// NOTE: GUI is the other way around
	glCullFace(GL_FRONT);

	m_renderQueue->Process();

	// Verify if scissor rectangle state was correctly restored
	assert(m_scissorRectangles.empty());

	delete m_renderQueue;
	m_renderQueue = nullptr;

	// Reset face culling mode
	glCullFace(GL_BACK);
}

void GUIRenderer::SetWindow(Window* window)
{
	if(m_window)
	{
		m_window->OnKeyRepeat.RemoveAll(this);
		m_window->OnTextInput.RemoveAll(this);
		m_window->OnTextComposition.RemoveAll(this);
		m_window = nullptr;
	}

	m_window = window;

	if(m_window)
	{
		m_window->OnKeyRepeat.Add(this, &GUIRenderer::m_OnKeyRepeat);
		m_window->OnTextInput.Add(this, &GUIRenderer::m_OnTextInput);
		m_window->OnTextComposition.Add(this, &GUIRenderer::m_OnTextComposition);
	}
}

Window* GUIRenderer::GetWindow() const
{
	return m_window;
}

void GUIRenderer::PushScissorRect(const Rect& scissor)
{
	if(!m_scissorRectangles.empty())
	{
		m_scissorRect = m_scissorRectangles.back().Clamp(scissor);
	}
	else
	{
		m_scissorRect = scissor;
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

void GUIRenderer::SetInputFocus(GUIElementBase* element)
{
	if(m_textInput.elementFocus)
	{
		if(m_textInput.elementFocus == element)
			return; // Already focused
		m_textInput.elementFocus->m_rendererFocus = nullptr;
	}

	m_textInput.elementFocus = element;
	if(m_window)
	{
		// Start/Stop allowing ime text input
		if(element)
		{
			element->m_rendererFocus = this;
			m_window->StartTextInput();
		}
		else
		{
			m_window->StopTextInput();
		}
	}
}
GUIElementBase* GUIRenderer::GetInputFocus() const
{
	return m_textInput.elementFocus;
}

const GUITextInput& GUIRenderer::GetTextInput() const
{
	return m_textInput;
}
Vector2i GUIRenderer::GetTextSize(const WString& str, uint32 fontSize /*= 16*/)
{
	Text text = font->CreateText(str, fontSize);
	return text->size;
}
Vector2i GUIRenderer::GetTextSize(const String& str, uint32 fontSize /*= 16*/)
{
	return GetTextSize(Utility::ConvertToWString(str), fontSize);
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
	return RenderText(rq, Utility::ConvertToWString(str), position, color, fontSize);
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

void GUIRenderer::m_OnTextInput(const WString& input)
{
	m_textInput.input += input;
}
void GUIRenderer::m_OnTextComposition(const TextComposition& input)
{
	m_textInput.composition = input.composition;
}
void GUIRenderer::m_OnKeyRepeat(Key key)
{
	if(key == Key::Backspace)
	{
		if(m_textInput.input.empty())
			m_textInput.backspaceCount++; // Send backspace
		else
		{
			auto it = m_textInput.input.end(); // Modify input string instead
			--it;
			m_textInput.input.erase(it);
		}
	}
}

void GUIRenderer::m_ResetTextInput()
{
	m_textInput.backspaceCount = 0;
	m_textInput.input.clear();
}

WString GUITextInput::Apply(const WString& in) const
{
	WString res = in + input;
	auto it = res.end();
	for(uint32 i = 0; i < backspaceCount; i++)
	{
		if(res.empty())
			break;
		--it;
		it = res.erase(it);
	}
	return res;
}

bool GUITextInput::HasChanges() const
{
	return input.size() != backspaceCount;
}
