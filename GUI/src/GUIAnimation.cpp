#include "stdafx.h"
#include "GUIAnimation.hpp"

GUIAnimation::GUIAnimation(GUIAnimationUpdater updater, float duration, Interpolation::TimeFunction timeFunction): m_duration(duration), updater(updater), timeFunction(timeFunction)
{
	assert(updater.IsBound());
}

bool GUIAnimation::Update(float deltaTime)
{
	if (m_time >= m_duration)
		return false;

	m_time += deltaTime;
	float r = m_time / m_duration;
	if (m_time >= m_duration)
	{
		r = 1.0f;
		m_time = m_duration;
	}

	updater.Call(timeFunction(reversed ? (1.0f - r) : r));

	return r < 1.0f;
}

float GUIAnimation::GetDuration() const
{
	return m_duration;
}
void GUIAnimation::SetDuration(float time)
{
	m_duration = time;
}

float GUIAnimation::GetPhase() const
{
	return m_time / m_duration;
}
void GUIAnimation::SetPhase(float time)
{
	m_time = time * m_duration;
}

float GUIAnimation::GetTime() const
{
	return m_time;
}
void GUIAnimation::SetTime(float time)
{
	m_time = time;
}
