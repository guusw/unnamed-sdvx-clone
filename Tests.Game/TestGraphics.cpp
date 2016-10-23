#include "stdafx.h"
#include "GraphicsBase.hpp"
#include <Graphics/RenderPass.hpp>
#include <Graphics/RenderQueue.hpp>

class RenderPassTest : public GUITestBase
{
public:
	virtual void Render(float deltaTime) override
	{
		Vector2 windowSize = m_window->GetWindowSize();
		RenderQueue& rq = guiRenderer->Begin(Rect(Vector2(), windowSize));
		{
			PointDrawCall* box = new PointDrawCall();
			box->mesh = guiRenderer->lineBox;
			box->worldTransform = Transform2D(Vector2(0), Vector2(100));
			box->mat = guiRenderer->colorMaterial;
			box->size = 2.0f;
			box->params.SetParameter("color", Color::Green);

			// Render a wire-frame quad to a render target/framebuffer
			Recti rtViewport = Recti(Vector2i(0, 0), rtSize);
			RenderState rs = RenderState::Create2DRenderState(rtViewport);
			RenderPass* rp = new RenderPass(m_gl, rs);
			rp->framebuffer = framebuffer;
			rp->Add(box);
			rq.Add(rp);

			// Render output from pass to fullscreen
			SimpleDrawCall* drawTexture = new SimpleDrawCall();
			drawTexture->mat = guiRenderer->textureMaterial;
			drawTexture->params.SetParameter("color", Color::White);
			drawTexture->params.SetParameter("mainTex", colorbuffer);
			drawTexture->worldTransform = Transform2D(Vector2(0.0f), windowSize);
			drawTexture->mesh = guiRenderer->guiQuad;
			rq.Add(drawTexture);
		}
		guiRenderer->End();
	}

	virtual void Init() override
	{
		GUITestBase::Init();

		framebuffer = FramebufferRes::Create(m_gl);
		colorbuffer = TextureRes::Create(m_gl);
		colorbuffer->Init(rtSize);
		framebuffer->AttachTexture(colorbuffer);
	}

	Vector2i rtSize = Vector2i(100);
	Framebuffer framebuffer;
	Texture colorbuffer;
};

Test("Graphics.RenderPass")
{
	RenderPassTest rpt;
	TestEnsure(rpt.Run());
}