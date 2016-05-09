#include "stdafx.h"
#include "Scoring.hpp"
#include "BeatmapPlayback.hpp"
#include <math.h>

const float Scoring::idleLaserMoveSpeed = 1.0f;
const MapTime Scoring::maxEarlyHitTime = 100;
const MapTime Scoring::perfectHitTime = 50;
const MapTime Scoring::maxLaserHitTime = 100;

Scoring::Scoring()
{
	currentComboCounter = 0;
	m_holdTickCounter = 0;
	m_lastTime = 0;
	laserPositions[0] = 0.0f;
	laserPositions[1] = 1.0f;
}
void Scoring::SetPlayback(BeatmapPlayback& playback)
{
	if(m_playback)
	{
		m_playback->OnObjectEntered.RemoveAll(this);
		m_playback->OnObjectLeaved.RemoveAll(this);
	}
	m_playback = &playback;
	m_playback->OnObjectEntered.Add(this, &Scoring::m_OnObjectEntered);
	m_playback->OnObjectLeaved.Add(this, &Scoring::m_OnObjectLeaved);
}
void Scoring::Tick(float deltaTime)
{
	assert(m_playback);
	Set<ObjectState*>& objects = m_playback->GetHittableObjects();
	MapTime time = m_playback->GetLastTime();
	MapTime delta = time - m_lastTime;
	if(delta <= 0)
		return;
	m_lastTime = time;

	// Count beats on current tick
	uint32 startBeat = 0;
	uint32 numBeats = m_playback->CountBeats(m_lastTime, delta, startBeat, 1);

	Vector<uint32> buttonsToAutoHit;

	// Perform objects in the hit queue
	for(auto it = objects.begin(); it != objects.end(); it++)
	{
		MapTime hitDelta = GetObjectHitDelta(*it);
		MultiObjectState* mobj = **it;
		// Check for hold breaks
		if(mobj->type == ObjectType::Hold)
		{
			HoldObjectState* hold = (HoldObjectState*)mobj;
			MapTime endTime = hold->duration + hold->time;
			MapTime endDelta = time - endTime;

			if(!activeHoldObjects[hold->index])
			{
				if(autoplay && hitDelta >= 0 && endDelta < 0)
					OnButtonPressed(hold->index);

				// Should be held down?, also check for release offset
				if(abs(hitDelta) > maxEarlyHitTime)
				{
					MapTime endTime = hold->duration + hold->time;
					MapTime endDelta = time - endTime;

					if(abs(endDelta) > maxEarlyHitTime)
					{
						// Combo break, released too early
						m_ResetCombo();
						//OnButtonMiss.Call(hold->index);
						continue;
					}
				}
			}
			else
			{
				const TimingPoint* tp = m_playback->GetTimingPointAt(hold->time);
				assert(tp);

				// Give combo points for hold note ticks
				uint32 comboTickLast = (uint32)floor(((double)lastHoldDuration[hold->index] * (double)tp->measure) / tp->beatDuration);
				// Clamp duration to make sure no one can get extra points on hold notes
				lastHoldDuration[hold->index] = Math::Min<MapTime>(lastHoldDuration[hold->index] + delta, hold->duration-1);
				uint32 comboTickCurrent = (uint32)floor(((double)lastHoldDuration[hold->index] * (double)tp->measure) / tp->beatDuration);
				if(comboTickCurrent > comboTickLast)
				{
					m_AddCombo();

					currentHitScore += 1;
					currentMaxScore += 1;
				}

				if(endDelta > 0)
				{
					OnButtonReleased(hold->index);
				}
			}
		}
		// Autoplay single notes
		else if(mobj->type == ObjectType::Single)
		{
			if(autoplay && hitDelta >= 0)
			{
				buttonsToAutoHit.Add(mobj->button.index);
			}
		}
		// Set the active laser segment
		else if(mobj->type == ObjectType::Laser)
		{
			uint32 laserIndex = mobj->laser.index;
			LaserObjectState* laser = (LaserObjectState*)mobj;
			MapTime endTime = laser->duration + laser->time;
			MapTime endDelta = time - endTime;

			if(!activeLaserObjects[laserIndex])
			{
				// Allow early hit on laser slams
				MapTime earlyHitWindow = 0;
				if((laser->flags & LaserObjectState::flag_Instant) != 0)
					earlyHitWindow = maxLaserHitTime;

				// Laser entered timing window
				if((time + earlyHitWindow) > mobj->time && endDelta < maxLaserHitTime)
				{
					// Initialy active
					laserActive[laserIndex] = true;
					activeLaserObjects[laserIndex] = (LaserObjectState*)mobj;
				}
			}
		}
	}

	// Autoplay button logic
	for(uint32& b : buttonsToAutoHit)
	{
		ObjectState* object = OnButtonPressed(b);
		if(!object)
			Log("Autoplay fail?", Logger::Warning);
	}

	// Tick lasers
	bool interpolateOutput = true;
	for(uint32 i = 0; i < 2; i++)
	{
		LaserObjectState* laser = activeLaserObjects[i];
		if(!laser)
			continue;

		auto AdvanceLaser = [&]()
		{
			// Remove current segment from hittable objects queue
			objects.erase(*laser);
			if(laser->next)
			{
				activeLaserObjects[i] = laser->next;
				laserHoldDuration[i] = 0;

				// Transfer active state to new segment
				if(laserActive[i] == true && (laser->next->flags & LaserObjectState::flag_Instant) == 0) 
				{
					laserHoldObjects[i] = activeLaserObjects[i];
				}
				else
				{
					laserHoldObjects[i] = nullptr;
				}
			}
			else
			{
				activeLaserObjects[i] = nullptr;
				laserActive[i] = false;
				laserHoldDuration[i] = 0;
				laserMissDuration[i] = 0;
			}
		};

		float laserTargetNew = m_SampleLaserPosition(time, activeLaserObjects[i]);
		laserTargetPositions[i] = laserTargetNew;

		// The direction in which the laser segment is moving
		float laserDelta = laser->points[1] - laser->points[0];

		// Current section hit timings
		MapTime hitDelta = time - laser->time;
		MapTime endTime = laser->duration + laser->time;
		MapTime endDelta = time - endTime;

		// Autoplay just mimics the wanted direction
		if(autoplay)
		{
			if(!laser->prev && hitDelta < 0)
				laserInput[i] = 0.0f; // Wait for the right moment
			else if(laserDelta == 0.0f)
				laserInput[i] = 0.0f;
			else
				laserInput[i] = (laserDelta < 0.0f) ? -1.0f : 1.0f;
		}

		// Whenether the user is holding the right direction
		bool isBeingControlled = Math::Sign(laserDelta) == Math::Sign(laserInput[i]);


		if((laser->flags & LaserObjectState::flag_Instant) != 0)
		{
			if(isBeingControlled && (!laser->prev || hitDelta > 0))
			{
				// Got it
				currentHitScore++; // 1 Point for laser slams
				currentMaxScore++;
				m_AddCombo();
				OnLaserSlamHit.Call(i);
				laserPositions[i] = laser->points[1];
				AdvanceLaser();
				interpolateOutput = false; // Instant filter changes

				// Register statistic
				hitStats.Add(HitStat(time, hitDelta));
			}
			else if(hitDelta > maxLaserHitTime)
			{
				// Miss laser slam
				m_ResetCombo();
				currentMaxScore++; // miss 1 Point for laser slams
				laserActive[i] = false;
				AdvanceLaser();

				// Register statistic
				hitStats.Add(HitStat(time, hitDelta));
			}
		}
		else
		{
			// Combo points for laser
			const TimingPoint* tp = m_playback->GetTimingPointAt(laser->time);
			assert(tp);
			// The last combo tick on this laser
			uint32 comboTickLast = (uint32)floor(((double)laserHoldDuration[laser->index] * (double)tp->measure) / tp->beatDuration);

			// Handle miss time
			if(!isBeingControlled)
			{
				if(laserDelta != 0.0f)
					laserMissDuration[i] += delta;
			}
			else
			{
				laserMissDuration[i] = 0;
			}

			if(endDelta > 0) // Advance
			{
				AdvanceLaser();
				laserHoldDuration[i] = 0;
			} 
			else if(isBeingControlled || (laserMissDuration[i] < maxLaserHitTime))
			{
				// Clamp duration to make sure no one can get extra points on hold notes
				laserHoldDuration[laser->index] = Math::Min<MapTime>(laserHoldDuration[laser->index] + delta, laser->duration - 1);
				// Make cursor follow laser
				laserPositions[i] = laserTargetNew;
				laserHoldObjects[i] = laser;
				laserActive[i] = true;

				// Give combo points
				uint32 comboTickCurrent = (uint32)floor(((double)laserHoldDuration[laser->index] * (double)tp->measure) / tp->beatDuration);
				if(comboTickCurrent > comboTickLast)
				{
					m_AddCombo();

					currentHitScore += 1;
					currentMaxScore += 1;
				}
			}
			else
			{
				// Combo break on laser segment
				m_ResetCombo();
				laserHoldDuration[i] = 0; 
				laserHoldObjects[i] = nullptr;
				laserActive[i] = false;
			}

		}

		// Advance to next slam segment if possible (Early hit)
		if(laser->next && (laser->next->flags & LaserObjectState::flag_Instant) != 0)
		{
			float nextDir = laser->next->points[1] - laser->next->points[0];
			if(Math::Sign(nextDir) != laserDelta)
			{
				MapTime delta = time - laser->next->time;
				if(nextDir == Math::Sign(laserInput[i]) && delta > -maxLaserHitTime)
				{
					AdvanceLaser();
				}
			}
		}
	}

	// Update the interpolation of laser output values
	m_UpdateLaserOutput(deltaTime, interpolateOutput);
}
ScoreHitRating Scoring::GetHitRatingFromDelta(MapTime delta)
{
	delta = abs(delta);
	if(delta > maxEarlyHitTime)
		return ScoreHitRating::Miss;
	if(delta > perfectHitTime)
		return ScoreHitRating::Good;
	return ScoreHitRating::Perfect;
}
ObjectState* Scoring::OnButtonPressed(uint32 buttonCode)
{
	assert(m_playback);
	Set<ObjectState*>& objects = m_playback->GetHittableObjects();
	MapTime time = m_playback->GetLastTime();

	// Find closest object to hit
	MapTime closestObject = INT_MAX;
	ObjectState* hitObject = nullptr;
	for(auto it = objects.begin(); it != objects.end();)
	{
		MultiObjectState* mobj = **it;
		if((*it)->type != ObjectType::Laser && mobj->button.index == buttonCode)
		{
			if((*it)->type == ObjectType::Single)
			{
				MapTime delta = mobj->time - time;
				if(abs(delta) < abs(closestObject))
				{
					closestObject = delta;
					hitObject = *it;
				}
			}
			else if((*it)->type == ObjectType::Hold)
			{
				MapTime delta = mobj->time - time;
				MapTime endDelta = (mobj->time + mobj->hold.duration) - time;
				if(abs(delta) < abs(closestObject))
				{
					closestObject = delta;
					hitObject = *it;
				}
				if(abs(endDelta) < abs(closestObject))
				{
					closestObject = delta;
					hitObject = *it;
				}
			}
		}
		it++;
	}

	if(hitObject)
	{
		MultiObjectState* mobj = *hitObject;
		if(hitObject->type == ObjectType::Hold)
		{
			activeHoldObjects[mobj->hold.index] = (HoldObjectState*)hitObject;
		}
		else if(hitObject->type == ObjectType::Single)
		{
			objects.erase(hitObject);
			m_RegisterHit(hitObject);
		}
		OnButtonHit.Call(buttonCode, hitObject);
	}

	return hitObject;
}
void Scoring::OnButtonReleased(uint32 buttonCode)
{
	if(activeHoldObjects[buttonCode])
	{
		activeHoldObjects[buttonCode] = nullptr;
		lastHoldDuration[buttonCode] = 0;
	}
}
void Scoring::m_RegisterHit(ObjectState* obj)
{
	MultiObjectState* mobj = *obj;
	MapTime delta = GetObjectHitDelta(obj);
	ScoreHitRating score = GetHitRatingFromDelta(delta);
	hitStats.emplace_back(obj->time, delta);
	currentMaxScore += (uint32)ScoreHitRating::Perfect;
	currentHitScore += (uint32)score;
	if(score != ScoreHitRating::Miss)
		m_AddCombo();
	else
	{
		m_ResetCombo();
		if(obj->type == ObjectType::Single)
		{
			OnButtonMiss.Call(mobj->button.index);
		}
	}
}
void Scoring::m_OnObjectEntered(ObjectState* obj)
{
}
void Scoring::m_OnObjectLeaved(ObjectState* obj)
{
	if(obj->type == ObjectType::Single)
	{
		m_RegisterHit(obj);
	}
}
MapTime Scoring::GetObjectHitDelta(ObjectState* obj)
{
	assert(m_playback);
	return (m_playback->GetLastTime() - obj->time);
}
void Scoring::m_AddCombo(uint32 amount)
{
	OnComboChanged.Call(currentComboCounter + amount);
	currentComboCounter += amount;
}
void Scoring::m_ResetCombo()
{
	OnComboChanged.Call(0);
	currentComboCounter = 0;
}

bool Scoring::IsActive(ObjectState* object) const
{
	MultiObjectState* mobj = *object;
	if(mobj->type == ObjectType::Hold)
	{
		if(activeHoldObjects[mobj->hold.index] == (HoldObjectState*)object)
			return true;
	}
	else if(mobj->type == ObjectType::Laser)
	{
		if(activeLaserObjects[mobj->laser.index])
		{
			return laserActive[mobj->laser.index];
		}
	}
	return false;
}
bool Scoring::IsLaserActive() const
{
	return laserActive[0] || laserActive[1];
}

float Scoring::GetLaserRollOutput(uint32 index)
{
	assert(index >= 0 && index <= 1);
	if(activeLaserObjects[index])
	{
		if(index == 0)
			return -laserTargetPositions[index];
		if(index == 1)
			return (1.0f - laserTargetPositions[index]);
	}
	return 0.0f;
}

static const float laserOutputInterpolationDuration = 0.2f;
float Scoring::GetLaserOutput()
{
	float f = Math::Min(1.0f, m_timeSinceOutputSet / laserOutputInterpolationDuration);
	return m_laserOutputSource + (m_laserOutputTarget - m_laserOutputSource) * f;
}
void Scoring::m_UpdateLaserOutput(float deltaTime, bool interpolate)
{
	m_timeSinceOutputSet += deltaTime;
	float v = m_GetLaserOutputRaw();
	float c = GetLaserOutput();
	if(v != c)
	{
		m_laserOutputTarget = v;
		m_laserOutputSource = c;
		m_timeSinceOutputSet = interpolate ? 0.0f : laserOutputInterpolationDuration;
	}
}
float Scoring::m_GetLaserOutputRaw()
{
	float val = 0.0f;
	for(int32 i = 0; i < 2; i++)
	{
		if(activeLaserObjects[i] && laserActive[i])
		{
			float actual = laserTargetPositions[i];
			// Undo laser extension
			if((activeLaserObjects[i]->flags & LaserObjectState::flag_Extended) != 0)
			{
				actual += 0.5f;
				actual *= 0.5f;
				assert(actual >= 0.0f && actual <= 1.0f);
			}
			if(i == 1) // Second laser goes the other way
				actual = 1.0f - actual;
			val = Math::Max(actual, val);
		}
	}
	return val;
}
float Scoring::m_SampleLaserPosition(MapTime time, LaserObjectState* laser)
{
	time -= laser->time;
	float r = Math::Clamp((float)time / Math::Max(1.0f, (float)laser->duration), 0.0f, 1.0f);
	return laser->points[0] + (laser->points[1] - laser->points[0]) * r;
}

