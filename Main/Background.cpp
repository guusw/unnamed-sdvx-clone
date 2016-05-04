#include "stdafx.h"
#include "Background.hpp"
#include "Application.hpp"
#include "RenderState.hpp"
#include "RenderQueue.hpp"
#include "Background.hpp"
#include "Game.hpp"
#include "Track.hpp"
#include "Camera.hpp"
#include "MeshGenerators.hpp"

/* Background template for fullscreen effects */
class FullscreenBackground : public Background
{
public:
	virtual bool Init() override
	{
		fullscreenMesh = MeshGenerators::Quad(g_gl, Vector2(-1.0f), Vector2(2.0f));
		return true;
	}
	void UpdateRenderState(float deltaTime)
	{
		renderState = g_application->GetRenderStateBase();
	}
	virtual void Render(float deltaTime) override
	{
		assert(fullscreenMaterial);

		// Render a fullscreen quad
		RenderQueue rq(g_gl, renderState);
		rq.Draw(Transform(), fullscreenMesh, fullscreenMaterial, fullscreenMaterialParams);
		rq.Process();
	}

	RenderState renderState;
	Mesh fullscreenMesh;
	Material fullscreenMaterial;
	MaterialParameterSet fullscreenMaterialParams;
};

class TestBackground : public FullscreenBackground
{
	virtual bool Init() override
	{
		if(!FullscreenBackground::Init())
			return false;

		CheckedLoad(fullscreenMaterial = g_application->LoadMaterial("background"));

		return true;
	}
	virtual void Render(float deltaTime) override
	{
		UpdateRenderState(deltaTime);

		Vector2 timing;
		const TimingPoint& tp = game->GetPlayback().GetCurrentTimingPoint();
		timing.x = game->GetPlayback().GetBarTime();

		// every 1/4 tick
		float tickTime = fmodf(timing.x * (float)tp.measure, 1.0f);
		//timing.y = powf(tickTime, 2);
		timing.y = powf(1.0f-tickTime, 1);
		//if(tickTime > 0.7f)
		//	timing.y += ((tickTime - 0.7f) / 0.3f) * 0.8f; // Gradual build up again

		Vector3 trackEndWorld = Vector3(0.0f, 25.0f, 0.0f);
		Vector2i screenCenter = game->GetCamera().Project(trackEndWorld);
		fullscreenMaterialParams.SetParameter("screenCenter", screenCenter);
		fullscreenMaterialParams.SetParameter("timing", timing);

		FullscreenBackground::Render(deltaTime);
	}
};

Background* CreateBackground(class Game* game)
{
	Background* bg = new TestBackground();
	bg->game = game;
	if(!bg->Init())
	{
		delete bg;
		return nullptr;
	}
	return bg;
}