#include "stdafx.h"
#include "Camera.hpp"
#include "Application.hpp"
#include "Track.hpp"

Camera::Camera()
{

}
Camera::~Camera()
{

}
void Camera::Tick(float deltaTime)
{
	// Smoothly blend to new roll value
	//float rollInterpolateSpeed = 0.9f;
	//m_baseRollBlend -= (m_baseRollBlend * rollInterpolateSpeed) * deltaTime;
	//m_baseRollBlend += (baseRoll * rollInterpolateSpeed) * deltaTime;

	// Like gravity but for roll
	float angle = m_roll * Math::pi;
	Vector3 rod = Vector3(sin(m_roll), cos(m_roll), 0.0f);
	Vector3 gravity = -Vector3(sin(baseRoll), cos(baseRoll), 0.0f);
	float rollForce = -VectorMath::Cross(gravity, rod).z * 10.0f;

	// Decay velocity
	m_rollVelocity -= m_rollVelocity * Math::Min(1.0f, 8.0f * deltaTime);

	// Apply force
	m_rollVelocity += rollForce * deltaTime;

	// Apply velocity
	m_roll += m_rollVelocity * deltaTime;
	m_roll = m_ClampRoll(m_roll);

	// Update camera shake effects
	m_shakeOffset = Vector3(0.0f);
	for(auto it = m_shakeEffects.begin(); it != m_shakeEffects.end();)
	{
		if(it->time <= 0.0f)
		{
			it = m_shakeEffects.erase(it);
			continue;
		}

		it->time -= deltaTime;
		it->offsets += Vector3(deltaTime);
		float shakeIntensity = it->time / it->duration;
		Vector3 shakeVec = it->amplitude;
		Vector3 shakeInput = it->frequency * it->offsets;
		shakeVec = Vector3(cos(shakeInput.x), cos(shakeInput.y), cos(shakeInput.z)) * shakeVec;
		m_shakeOffset += shakeVec * shakeIntensity;
		it++;
	}
}
void Camera::AddCameraShake(CameraShake cameraShake)
{
	// Randomize offsets
	cameraShake.offsets = Vector3(
		Random::FloatRange(0.0f, 1.0f / cameraShake.frequency.x),
		Random::FloatRange(0.0f, 1.0f / cameraShake.frequency.y),
		Random::FloatRange(0.0f, 1.0f / cameraShake.frequency.z));
	m_shakeEffects.Add(cameraShake);
}
void Camera::AddRollImpulse(float dir, float strength)
{
	m_rollVelocity += dir * strength;
}

RenderState Camera::CreateRenderState(bool clipped)
{
	float viewRangeExtension = clipped ? 0.0f : 5.0f;

	RenderState rs = g_application->GetRenderStateBase();

	// Calculate Track positioning based on zoom values
	static const float nearDistBase = 5.0f;
	static const float nearDistZoom = 2.0f;
	static const float maxNearPlane = 0.2f;
	static const float heightMin = 0.3f;
	static const float heightBase = 0.9f;
	static const float heightMax = 1.5f;

	float cameraTilt = zoomTop * 10.0f + 2.0f;
	float cameraHeight = heightBase;
	float nearDist = nearDistBase;
	if(zoomBottom < 0)
	{
		cameraHeight -= Math::Max(-1.0f, zoomBottom) * (heightMin - heightBase);
	}
	else
	{
		cameraHeight += Math::Min(1.0f, zoomBottom) * (heightMax - heightBase);
	}

	nearDist = nearDistBase + zoomBottom * nearDistZoom;


	Transform cameraTransform;
	float nearDistance = Math::Max(maxNearPlane, nearDist - viewRangeExtension);
	float farDistance = nearDistance + track->trackLength + viewRangeExtension;
	cameraTransform *= Transform::Rotation({ 0.0f, 0.0f, m_roll * 360.0f });
	cameraTransform *= Transform::Translation(m_shakeOffset + Vector3( 0.0f, -cameraHeight, -nearDist ));
	cameraTransform *= Transform::Rotation({ -90.0f + cameraTilt, 0.0f, 0.0f });

	rs.cameraTransform = cameraTransform;
	rs.projectionTransform = ProjectionMatrix::CreatePerspective(30.0f, g_aspectRatio, nearDistance, farDistance);

	return rs;
}

float Camera::m_ClampRoll(float in) const
{
	float ain = abs(in);
	if(ain < 1.0f)
		return in;
	bool odd = ((uint32)abs(in) % 2) == 1;
	int32 sign = Math::Sign(in);
	if(odd)
	{
		// Swap sign and modulo
		return -sign * (1.0f-fmodf(ain, 1.0f));
	}
	else
	{
		// Keep sign and modulo
		return sign * fmodf(ain, 1.0f);
	}
}

CameraShake::CameraShake(float duration) : duration(duration)
{
	time = duration;
}
CameraShake::CameraShake(float duration, float amplitude, float freq) : duration(duration), amplitude(amplitude), frequency(freq)
{
	time = duration;
}

