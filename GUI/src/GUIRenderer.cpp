#include "stdafx.h"
#include "GUIRenderer.hpp"
#include "GUIRenderData.hpp"
#include "GUI.hpp"

GUIRenderer::~GUIRenderer()
{
	assert(!m_renderQueue);
	SetInputFocus(nullptr);
	SetWindow(nullptr);
}
bool GUIRenderer::Init(class OpenGL* gl, class Graphics::Window* window)
{
	assert(gl);
	m_gl = gl;

	// Font
	CheckedLoad(font = FontRes::Create(gl, "fonts/segoeui.ttf"));

	auto LoadMaterial = [&](const String& name)
	{
		String basePath = String("shaders/");
		String vs = Path::Normalize(basePath + name + ".vs");
		String fs = Path::Normalize(basePath + name + ".fs");
		return MaterialRes::Create(gl, vs, fs);
	};

	// Load GUI shaders
	CheckedLoad(fontMaterial = LoadMaterial("font"));
	fontMaterial->opaque = false;
	CheckedLoad(textureMaterial = LoadMaterial("guiTex"));
	textureMaterial->opaque = false;
	CheckedLoad(colorMaterial = LoadMaterial("guiColor"));
	colorMaterial->opaque = false;
	CheckedLoad(buttonMaterial = LoadMaterial("guiButton"));
	buttonMaterial->opaque = false;

	guiQuad = MeshGenerators::Quad(m_gl, Vector2(0, 0), Vector2(1, 1));

	pointMesh = MeshRes::Create(m_gl);
	Vector<MeshGenerators::SimpleVertex> points = { MeshGenerators::SimpleVertex(Vector3(0,0,0), Vector2(0,0)) };
	pointMesh->SetData(points);
	pointMesh->SetPrimitiveType(PrimitiveType::PointList);

	// Initial window assignment
	SetWindow(window);

	return true;
}
void GUIRenderer::Render(float deltaTime, Rect viewportSize, Ref<class GUIElementBase> rootElement)
{
	m_viewportSize = viewportSize;

	Begin();

	// Update mouse input
	Vector2i newMouse = m_window->GetMousePos();
	m_mouseDelta = newMouse - m_mousePos;
	m_mousePos = newMouse;

	// Render GUI
	GUIRenderData grd;
	grd.rq = m_renderQueue;
	grd.guiRenderer = this;
	grd.deltaTime = deltaTime;
	grd.area = viewportSize;
	grd.transform = Transform();

	// Handle input focus, position calculation, etc.
	GUIElementBase* inputElement = nullptr;
	rootElement->PreRender(grd, inputElement);
	m_hoveredElement = inputElement;

	// Clear input focus?
	if(!m_hoveredElement && GetMouseButtonPressed(MouseButton::Left))
	{
		SetInputFocus(nullptr);
	}

	// Render
	rootElement->Render(grd);

	// Clear text input after processing
	m_ResetTextInput();

	// Clear mouse scroll state
	m_mouseScrollDelta = 0;

	// Shift mouse button state
	memcpy(m_mouseButtonStateLast, m_mouseButtonState, sizeof(float)*3);

	End();
}

Graphics::RenderQueue& GUIRenderer::Begin()
{
	// Must have not called begin before this / or have called end
	assert(m_renderQueue == nullptr);

	// Set initial scissor rect to be disabled
	m_scissorRect = Rect(Vector2(0, 0), Vector2(-1));

	// Render state/queue for the GUI
	Vector2 windowSize = m_window->GetWindowSize();
	RenderState guiRs;
	guiRs.viewportSize = windowSize;
	guiRs.projectionTransform = ProjectionMatrix::CreateOrthographic(0, windowSize.x, windowSize.y, 0.0f, -1.0f, 100.0f);
	guiRs.time = 0.0f;
	guiRs.aspectRatio = windowSize.y / windowSize.x;
	m_renderQueue = new RenderQueue(m_gl, guiRs);

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

void GUIRenderer::SetWindow(Graphics::Window* window)
{
	if(m_window)
	{
		m_window->OnKeyRepeat.RemoveAll(this);
		m_window->OnTextInput.RemoveAll(this);
		m_window->OnTextComposition.RemoveAll(this);
		m_window->OnKeyPressed.RemoveAll(this);
		m_window->OnMousePressed.RemoveAll(this);
		m_window->OnMouseReleased.RemoveAll(this);
		m_window->OnMouseScroll.RemoveAll(this);
		m_window = nullptr;
	}

	m_window = window;

	if(m_window)
	{
		m_window->OnKeyRepeat.Add(this, &GUIRenderer::m_OnKeyRepeat);
		m_window->OnTextInput.Add(this, &GUIRenderer::m_OnTextInput);
		m_window->OnTextComposition.Add(this, &GUIRenderer::m_OnTextComposition);
		m_window->OnKeyPressed.Add(this, &GUIRenderer::m_OnKeyPressed);
		m_window->OnMousePressed.Add(this, &GUIRenderer::m_OnMousePressed);
		m_window->OnMouseReleased.Add(this, &GUIRenderer::m_OnMouseReleased);
		m_window->OnMouseScroll.Add(this, &GUIRenderer::m_OnMouseScroll);
	}
}

Graphics::Window* GUIRenderer::GetWindow() const
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
		m_scissorRect = m_viewportSize.Clamp(scissor);
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
		return m_viewportSize;
	}
	else 
	{
		return m_scissorRectangles.back();
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
Vector2i GUIRenderer::RenderText(const WString& str, const Vector2& position, const Color& color /*= Color(1.0f)*/, uint32 fontSize /*= 16*/)
{
	if(m_scissorRect.size.x == 0 || m_scissorRect.size.y == 0)
		return Vector2i(0, 0);

	Text text = font->CreateText(str, fontSize);
	Transform textTransform;
	textTransform *= Transform::Translation(position);
	MaterialParameterSet params;
	params.SetParameter("color", color);
	m_renderQueue->DrawScissored(m_scissorRect, textTransform, text, fontMaterial, params);
	return text->size;
}
Vector2i GUIRenderer::RenderText(const String& str, const Vector2& position, const Color& color /*= Color(1.0f)*/, uint32 fontSize /*= 16*/)
{
	return RenderText(Utility::ConvertToWString(str), position, color, fontSize);
}
void GUIRenderer::RenderText(Text& text, const Vector2& position, const Color& color /*= Color(1.0f)*/)
{
	if(m_scissorRect.size.x == 0 || m_scissorRect.size.y == 0)
		return;

	Transform textTransform;
	textTransform *= Transform::Translation(position);
	MaterialParameterSet params;
	params.SetParameter("color", color);
	m_renderQueue->DrawScissored(m_scissorRect, textTransform, text, fontMaterial, params);
}
void GUIRenderer::RenderRect(const Rect& rect, const Color& color /*= Color(1.0f)*/, Texture texture /*= Texture()*/)
{
	if(m_scissorRect.size.x == 0 || m_scissorRect.size.y == 0)
		return;

	Transform transform;
	transform *= Transform::Translation(rect.pos);
	transform *= Transform::Scale(Vector3(rect.size.x, rect.size.y, 1.0f));
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
void GUIRenderer::RenderButton(const Rect& rect, Texture texture, Margini border, const Color& color /*= Color::White*/)
{
	if(m_scissorRect.size.x == 0 || m_scissorRect.size.y == 0)
		return;

	Transform transform;
	transform *= Transform::Translation(rect.pos);
	MaterialParameterSet params;
	params.SetParameter("color", color);
	params.SetParameter("mainTex", texture);

	// Calculate border offsets
	Rect r2 = border.Apply(Recti(rect));
	Vector2 size = texture->GetSize();

	Vector2 tl = (r2.pos - rect.pos) / rect.size;
	Vector2 br = (r2.size + r2.pos - rect.pos) / rect.size;
	Vector4 borderCoords = Vector4(tl.x, tl.y, br.x, br.y);

	// Texture border coords
	Vector2 textl = Vector2((float)border.left, (float)border.top) / size;
	Vector2 texbr = Vector2(1.0f) - Vector2((float)border.right, (float)border.bottom) / size;
	Vector4 texBorderCoords = Vector4(textl.x, textl.y, texbr.x, texbr.y);

	params.SetParameter("border", borderCoords);
	params.SetParameter("texBorder", texBorderCoords);
	params.SetParameter("size", rect.size);
	params.SetParameter("texSize", size);

	m_renderQueue->DrawScissored(m_scissorRect, transform, pointMesh, buttonMaterial, params);
}

const Vector2i& GUIRenderer::GetMousePos() const
{
	return m_mousePos;
}
const Vector2i& GUIRenderer::GetMouseDelta() const
{
	return m_mouseDelta;
}

bool GUIRenderer::GetMouseButton(MouseButton btn) const
{
	return m_mouseButtonState[(size_t)btn];
}
bool GUIRenderer::GetMouseButtonPressed(MouseButton btn) const
{
	return m_mouseButtonState[(size_t)btn] && !m_mouseButtonStateLast[(size_t)btn];
}
bool GUIRenderer::GetMouseButtonReleased(MouseButton btn) const
{
	return !m_mouseButtonState[(size_t)btn] && m_mouseButtonStateLast[(size_t)btn];
}

int32 GUIRenderer::GetMouseScroll() const
{
	return m_mouseScrollDelta;
}

GUIElementBase* GUIRenderer::GetHoveredElement()
{
	return m_hoveredElement;
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
void GUIRenderer::m_OnKeyPressed(Key key)
{
	if(key == Key::V)
	{
		if(m_window->GetModifierKeys() == ModifierKeys::Ctrl)
		{
			if(m_window->GetTextComposition().composition.empty())
			{
				// Paste clipboard text into input buffer
				m_textInput.input += m_window->GetClipboard();
			}
		}
	}
}
void GUIRenderer::m_OnMousePressed(MouseButton btn)
{
	m_mouseButtonState[(size_t)btn] = true;
}
void GUIRenderer::m_OnMouseReleased(MouseButton btn)
{
	m_mouseButtonState[(size_t)btn] = false;
}
void GUIRenderer::m_OnMouseScroll(int32 scroll)
{
	m_mouseScrollDelta += scroll;
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

bool GUIRenderData::OverlapTest(Rect rect) const
{
	Vector2 mouse = guiRenderer->GetMousePos();
	// Overlap the given element
	if(!GUIElementBase::OverlapTest(rect, mouse))
		return false;
	// Additionally check scissor rectangle
	return GUIElementBase::OverlapTest(guiRenderer->GetScissorRect(), mouse);
}
