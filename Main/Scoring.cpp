#include "stdafx.h"
#include "Scoring.hpp"
#include "BeatmapPlayback.hpp"
#include <math.h>

const float Scoring::idleLaserMoveSpeed = 1.0f;
const MapTime Scoring::maxEarlyHitTime = 100;
const MapTime Scoring::perfectHitTime = 50;
const MapTime Scoring::laserMissTreshold = perfectHitTime;

Scoring::Scoring()
{
	currentComboCounter = 0;
	m_hitNotesDelta = 0;
	m_numNotesHit = 0;
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
						currentComboCounter = 0;
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
				uint32 comboTickLast = (uint32)round(((double)lastHoldDuration[hold->index] * (double)tp->measure) / tp->beatDuration);
				lastHoldDuration[hold->index] += delta;
				uint32 comboTickCurrent = (uint32)round(((double)lastHoldDuration[hold->index] * (double)tp->measure) / tp->beatDuration);
				if(comboTickCurrent > comboTickLast)
				{
					currentComboCounter++;

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

			if(activeLaserObjects[laserIndex] != laser)
			{
				// Laser entered perfect hit area
				if(time >= mobj->time && endDelta < 0)
				{
					if(!laser->prev)
					{
						// Set initial pointer position
						laserPositions[laserIndex] = mobj->laser.points[0];
					}
					laserSlamHit[laserIndex] = false;
					activeLaserObjects[laserIndex] = (LaserObjectState*)mobj;
				}
			}
		}
	}

	// Autoplay logic
	for(uint32& b : buttonsToAutoHit)
	{
		ObjectState* object = OnButtonPressed(b);
		if(!object)
			Log("Autoplay fail?", Logger::Warning);
	}

	for(uint32 i = 0; i < 2; i++)
	{
		LaserObjectState* laser = activeLaserObjects[i];
		if(!laser)
			continue;

		float laserTargetNew = 0.0f;
		if(activeLaserObjects[i])
			laserTargetNew = m_SampleLaserPosition(time, activeLaserObjects[i]);
		float targetDelta = laserTargetNew - laserTargetPositions[i];
		laserTargetPositions[i] = laserTargetNew;

		MapTime endTime = laser->duration + laser->time;
		MapTime endDelta = time - endTime;
		if(endDelta > 0)
		{
			// Laser has passed
			activeLaserObjects[i] = nullptr;
			continue;
		}

		// Check if laser slam was hit
		if((laser->flags & LaserObjectState::flag_Instant) != 0 && !laserSlamHit[i])
		{
			OnLaserSlamHit.Call(i);
			laserSlamHit[i] = true;
		}

		if(autoplay)
		{
			laserPositions[i] = laserTargetPositions[i];
		}

	}
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
			return laserMissDuration[mobj->laser.index] < laserMissTreshold;
		}
	}
	return false;
}

float Scoring::GetActiveLaserRoll(uint32 index)
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
void Scoring::m_RegisterHit(ObjectState* obj)
{
	MultiObjectState* mobj = *obj;
	MapTime delta = GetObjectHitDelta(obj);
	ScoreHitRating score = GetHitRatingFromDelta(delta);
	hitStats.emplace_back(obj->time, delta);
	currentMaxScore += (uint32)ScoreHitRating::Perfect;
	currentHitScore += (uint32)score;
	if(score != ScoreHitRating::Miss)
		currentComboCounter++;
	else
	{
		currentComboCounter = 0;
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
float Scoring::m_SampleLaserPosition(MapTime time, LaserObjectState* laser)
{
	time -= laser->time;
	float r = Math::Clamp((float)time / Math::Max(1.0f, (float)laser->duration), 0.0f, 1.0f);
	return laser->points[0] + (laser->points[1] - laser->points[0]) * r;
}

