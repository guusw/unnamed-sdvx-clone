#pragma once
#include <Graphics/ResourceTypes.hpp>
#include <Graphics/ParticleEmitter.hpp>

/*
	Particles system
	contains emitters and handles the cleanup/lifetime of them.
*/
class ParticleSystemRes
{
public:
	virtual ~ParticleSystemRes() = default;
	static Ref<ParticleSystemRes> Create(class OpenGL* gl);
public:
	virtual Ref<ParticleEmitter> AddEmitter() = 0;
	virtual void Render(const class RenderState& rs, float deltaTime) = 0;
};

typedef Ref<ParticleSystemRes> ParticleSystem;

DEFINE_RESOURCE_TYPE(ParticleSystem, ParticleSystemRes);