#include "stdafx.h"
#include "Test.hpp"
#include "Application.hpp"
#include "Profiling.hpp"
#include "Scoring.hpp"
#include "Audio.hpp"
#include "Track.hpp"
#include "Camera.hpp"
#include "Background.hpp"

class Test_Impl : public Test
{
	ParticleSystem ps;
	Material particleMaterial;
	Texture particleTexture;

	Ref<ParticleEmitter> trailA;
	Ref<ParticleEmitter> trailB;
public:
	Ref<ParticleEmitter> CreateTrailEmitter(const Color& color)
	{
		Ref<ParticleEmitter> emitter = ps->AddEmitter();
		emitter->material = particleMaterial;
		emitter->texture = particleTexture;
		emitter->loops = 0;
		emitter->duration = 5.0f;
		emitter->SetSpawnRate(PPRange<float>(60, 80));
		emitter->SetStartPosition(PPBox({ 0.3f, 0.0f, 0.1f }));
		emitter->SetStartSize(PPRandomRange<float>(0.3f, 0.4f));
		emitter->SetFadeOverTime(PPRangeFadeIn<float>(1.0f, 0.0f, 0.4f));
		emitter->SetLifetime(PPRandomRange<float>(0.27f, 0.3f));
		emitter->SetStartDrag(PPConstant<float>(0.0f));
		emitter->SetStartVelocity(PPConstant<Vector3>({ 0,1,0 }));
		emitter->SetStartColor(PPConstant<Color>(color));
		return emitter;
	}
	Ref<ParticleEmitter> CreateExplosionEmitter(const Color& color, const Vector3 dir)
	{
		Ref<ParticleEmitter> emitter = ps->AddEmitter();
		emitter->material = particleMaterial;
		emitter->texture = particleTexture;
		emitter->loops = 1;
		emitter->duration = 0.2f;
		emitter->SetSpawnRate(PPRange<float>(200, 0));
		emitter->SetStartPosition(PPSphere(0.1f));
		emitter->SetStartSize(PPRandomRange<float>(0.7f, 1.1f));
		emitter->SetFadeOverTime(PPRangeFadeIn<float>(0.9f, 0.0f, 0.0f));
		emitter->SetLifetime(PPRandomRange<float>(0.22f, 0.3f));
		emitter->SetStartDrag(PPConstant<float>(0.2f));
		emitter->SetSpawnVelocityScale(PPRandomRange<float>(1.0f, 4.0f));
		emitter->SetScaleOverTime(PPRange<float>(1.0f, 0.4f));
		emitter->SetStartVelocity(PPConstant<Vector3>(dir * 5.0f));
		emitter->SetStartColor(PPConstant<Color>(color));
		return emitter;
	}
	bool Init()
	{
		// Test particle system
		ps = ParticleSystemRes::Create(g_gl);
		particleMaterial = g_application->LoadMaterial("particle");
		particleMaterial->blendMode = MaterialBlendMode::Additive;
		particleMaterial->opaque = false;
		particleTexture = g_application->LoadTexture("ParticleFlare.png");

		trailA = CreateTrailEmitter(Color::FromHSV(0, 0.7f, 1.0f));
		trailA->position = {-1,0,0};
		trailB = CreateTrailEmitter(Color::FromHSV(180, 0.7f, 1.0f));
		trailB->position = { 1,0,0 };

		return true;
	}
	Timer t;
	uint32 d = 0;
	virtual void Render(float deltaTime) override
	{
		if(t.SecondsAsFloat() > 0.8f)
		{
			Vector3 dir = ((d%2) == 0) ? Vector3(1, 0, 0) : Vector3(-1, 0, 0);
			Color c = ((d % 4) < 2) ? Color::FromHSV(0, 0.7f, 1.0f) : Color::FromHSV(180, 0.7f, 1.0f);
			CreateExplosionEmitter(c, dir);
			d++;
			t.Restart();
		}

		RenderState rs = g_application->GetRenderStateBase();
		rs.cameraTransform *= Transform::Rotation(-Vector3(-20.0f, 0.0f, 0.0f));
		rs.cameraTransform *= Transform::Translation(-Vector3(0.0f, 10.0f, 20.0f));
		rs.projectionTransform = ProjectionMatrix::CreatePerspective(40.0f, g_aspectRatio, 0.1f, 100.0f);
		RenderQueue rq(g_gl, rs);

		glClearColor(0.0f, 0.0f, 0.0f, 0);
		glClear(GL_COLOR_BUFFER_BIT);
		ps->Render(rs, deltaTime);
	}
};

Test* Test::Create()
{
	Test_Impl* impl = new Test_Impl();
	if(!impl->Init())
	{
		delete impl;
		return nullptr;
	}
	return impl;
}
