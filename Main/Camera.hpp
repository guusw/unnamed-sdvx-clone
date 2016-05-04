#pragma once
#include "RenderState.hpp"

/*
	Camera shake effect 
*/
struct CameraShake
{
	CameraShake(float duration);
	CameraShake(float duration, float amplitude, float freq);;
	Vector3 amplitude;
	Vector3 frequency;
	Vector3 offsets;
	float duration;
	float time = 0.0f;
};

/*
	Camera that hovers above the playfield track and can process camera shake and tilt effects
*/
class Camera
{
public:
	Camera();
	~Camera();

	// Updates the camera's shake effects, movement, etc.
	void Tick(float deltaTime);

	void AddCameraShake(CameraShake camerShake);
	void AddRollImpulse(float dir, float strength);

	// Generates a new render state for drawing from this cameras Point of View
	// the clipped boolean indicates whenether to clip the cameras clipping planes to the track range
	RenderState CreateRenderState(bool clipped);

	// The track being watched
	class Track* track;

	// Base roll value
	float baseRoll = 0.0f;

	// Zoom values, both can range from -1 to 1 to control the track zoom
	float zoomBottom = 0.0f;
	float zoomTop = 0.0f;

private:
	float m_baseRollBlend = 0.0f;
	float m_ClampRoll(float in) const;
	// -1 to 1 roll value
	float m_roll = 0.0f;
	// Roll force
	float m_rollVelocity = 0.0f;

	Vector<CameraShake> m_shakeEffects;
	// Base position with shake effects applied after a frame
	Vector3 m_shakeOffset;
};