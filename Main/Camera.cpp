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
	// Caluclate new force for the camera roll rotation
	// Like gravity but for roll
	float angle = m_roll * Math::pi;
	Vector3 wanted = Vector3(sin(m_targetRoll), cos(m_targetRoll), 0.0f);
	Vector3 rod = Vector3(sin(m_roll), cos(m_roll), 0.0f);
	float rollForce = VectorMath::Cross(wanted, rod).z * 100.0f;

	// Decay roll velocity
	m_rollVelocity -= m_rollVelocity * Math::Min(1.0f, 8.0f * deltaTime);

	float rollDelta = m_targetRoll - m_roll;
	//if(m_targetRollSet && abs(rollDelta) < 0.0001f) // Apply directly if delta is small enough
	//{
	//	m_roll = m_targetRoll;
	//}
	//else // Apply regular force otherwise
	{
		// Apply force
		m_rollVelocity += rollForce * deltaTime;

		// Apply velocity
		m_roll += m_rollVelocity * deltaTime;
		m_roll = m_ClampRoll(m_roll);
	}

	m_targetRollSet = false;
	m_targetRoll = 0.0f;

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

Vector2 Camera::Project(const Vector3& pos)
{
	Vector3 cameraSpace = m_rsLast.cameraTransform.TransformPoint(pos);
	Vector3 screenSpace = m_rsLast.projectionTransform.TransformPoint(cameraSpace);
	screenSpace.y = -screenSpace.y;
	screenSpace *= 0.5f;
	screenSpace += Vector2(0.5f, 0.5f);
	screenSpace *= m_rsLast.viewportSize;
	return screenSpace.xy();
}

RenderState Camera::CreateRenderState(bool clipped)
{
	float viewRangeExtension = clipped ? 0.0f : 5.0f;

	RenderState rs = g_application->GetRenderStateBase();

	// Clamp zoom values
	zoomBottom = Math::Clamp(zoomBottom, -1.0f, 1.0f);
	zoomTop = Math::Clamp(zoomTop, -1.0f, 1.0f);

	// Calculate Track positioning based on zoom values
	static const float nearDistBase = 4.3f;
	static const float nearDistZoom = 1.0f;
	static const float maxNearPlane = 0.2f;

	float cameraHeight = 0.8f;
	float nearDist = nearDistBase;

	// Trying to calculate tilt and camera yz position based on top/bottom zoom values
	float cameraTilt = -zoomTop * 10.0f + 4.0f;
	float zoomBottomMult = 1.0f;
	cameraHeight += -zoomBottom * zoomBottomMult * 0.3f;
	nearDist += -zoomBottom * zoomBottomMult* nearDistZoom;
	if(zoomBottom < 0)
		cameraTilt += 8.0f * -zoomBottom * zoomBottomMult;

	Transform cameraTransform;
	float nearDistance = Math::Max(maxNearPlane, nearDist - viewRangeExtension);
	float farDistance = nearDistance + track->trackLength + viewRangeExtension;
	cameraTransform *= Transform::Rotation({ 0.0f, 0.0f, m_roll * 360.0f });
	cameraTransform *= Transform::Translation(m_shakeOffset + Vector3( 0.0f, -cameraHeight, -nearDist ));
	cameraTransform *= Transform::Rotation({ -90.0f + cameraTilt, 0.0f, 0.0f });

	rs.cameraTransform = cameraTransform;
	rs.projectionTransform = ProjectionMatrix::CreatePerspective(27.0f, g_aspectRatio, nearDistance, farDistance);

	m_rsLast = rs;

	return rs;
}

void Camera::SetTargetRoll(float target)
{
	m_targetRoll = target;
	m_targetRollSet = true;
}

float Camera::m_ClampRoll(float in) const
{
	float ain = abs(in);
	if(ain < 1.0f)
		return in;
	bool odd = ((uint32)abs(in) % 2) == 1;
	float sign = Math::Sign(in);
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

