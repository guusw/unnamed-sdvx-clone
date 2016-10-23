#pragma once
#include <Shared/Interpolation.hpp>
#include <Shared/Action.hpp>

using Interpolation::TimeFunction;

// void func(float time); where time [0,1]
typedef Action<void, float> GUIAnimationUpdater;

// GUI Animation object
class GUIAnimation : public RefCounted<GUIAnimation>
{
public:
	GUIAnimation(GUIAnimationUpdater updater, float duration, Interpolation::TimeFunction timeFunction);

	// Return false when done
	bool Update(float deltaTime);

	float GetDuration() const;
	void SetDuration(float time);

	float GetPhase() const;
	void SetPhase(float time);

	float GetTime() const;
	void SetTime(float time);

	// Reversed animation
	bool reversed = false;

	// Timing function used
	TimeFunction timeFunction = Interpolation::Linear;

	// Updater function
	// Input = Time [0,1], already remapped to new time function
	GUIAnimationUpdater updater;

private:
	float m_time = 0.0f;
	float m_duration;
};