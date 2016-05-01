#include "stdafx.h"
#include "Scoring.hpp"

const float Scoring::idleLaserMoveSpeed = 1.0f;

const int64 Scoring::maxEarlyHitTime = 100;
const int64 Scoring::perfectHitTime = 50;

Scoring::Scoring()
{
	laserPositions[0] = 0.0f;
	laserPositions[1] = 1.0f;

	currentComboCounter = 0;
	m_hitNotesDelta = 0;
	m_numNotesHit = 0;
}
void Scoring::Tick(Vector<ObjectState*> objects, MapTime currentTime, float deltaTime)
{
	m_hitableObjects = objects;
	int64 timeDeltaMs = Math::Max<int64>(0, currentTime - m_currentTime);
	m_currentTime = currentTime;

	// Add objects in range to the array and find missed objects (>max hit time)
	m_hitableObjects.clear();
	for(auto& o : objects) 
	{
		int64 delta = currentTime - o->time;
		if(delta > maxEarlyHitTime) // Miss button in this object that were not hit
		{
			bool containsHoldNotes = false;
			for(uint32 i = 0; i < 6; i++)
			{
				ButtonState& btn = o->buttons[i];
				if(btn.on)
				{
					// Instant miss on single notes
					if(btn.duration == -1)
					{
						if(m_RecordHit(o, i))
						{
							m_AddScore(o, i, delta);
							OnButtonHit.Call(o, i);
							hitStats.Add({ currentTime, delta });
						}
					}
					else
					{
						// Miss hold note
						if(!currentHoldObjects[i])
						{
							if(m_RecordHit(o, i))
							{
								m_AddScore(o, i, delta);
								OnButtonHit.Call(o, i);
								hitStats.Add({ currentTime, delta });
							}
						}

						// Add as hitable object if there is still a hold not that can be triggered for more duration
						int64 holdDelta = (int64)currentTime - (int64)(btn.duration + o->time);
						if(holdDelta < 0)
						{
							containsHoldNotes = true;
						}
					}
				}
			}

			// Add if hold notes are in this object
			if(containsHoldNotes)
				m_hitableObjects.Add(o);
			continue;
		}
		if(o->time < (currentTime + maxEarlyHitTime))
			m_hitableObjects.Add(o);
	}

	// remove old recent hits
	for(auto it = recentHits.begin(); it != recentHits.end();)
	{
		int64 delta = it->first->time - currentTime;
		if(delta < -2000)
		{
			it = recentHits.erase(it);
			continue;
		}
		it++;
	}

	// Evaluate hold objects
	for(uint32 i = 0; i < 6; i++)
	{
		if(!currentHoldObjects[i])
			continue;
		// If button is pressed, add hold duration to note
		if(buttonStates[i])
			holdDurations[i] += timeDeltaMs;

		ButtonState& btn = currentHoldObjects[i]->buttons[i];
		MapTime end = currentHoldObjects[i]->time + btn.duration;
		// Evaluate the clearing of the hold object
		if(m_currentTime >= end)
		{
			m_TerminateHoldObject(i);
		}
	}

	for(uint32 i = 0; i < 2; i++)
	{
		laserPositions[i] = Math::Clamp(laserPositions[i] + laserInput[i] * idleLaserMoveSpeed * deltaTime, 0.0f, 1.0f);
	}
}
void Scoring::HandleButtonPress(uint32 buttonCode)
{
	assert(buttonCode >= 0 || buttonCode <= 6);
	buttonStates[buttonCode] = true;
	// Scan for single hittable or holdable objects
	for(uint32 i = 0; i < m_hitableObjects.size(); i++)
	{
		ObjectState* obj = m_hitableObjects[i];
		ButtonState& btn = m_hitableObjects[i]->buttons[buttonCode];
		int64 delta = m_hitableObjects[i]->time - m_currentTime;
		if(delta > maxEarlyHitTime)
			continue;
		if(btn.on)
		{

			if(btn.duration != -1)
			{
				assert(!currentHoldObjects[buttonCode]);
				// Hold object
				currentHoldObjects[buttonCode] = obj;
				holdDurations[buttonCode] = 0;
				holdStartDelta[buttonCode] = delta;
				break;
			}

			if(!m_RecordHit(obj, buttonCode))
				continue;

			OnButtonHit.Call(obj, buttonCode);
			m_AddScore(m_hitableObjects[i], buttonCode, delta);
			hitStats.Add({ m_currentTime, delta });
			m_hitNotesDelta += delta;
			m_numNotesHit++;
			break;
		}
	}
}
void Scoring::HandleButtonRelease(uint32 buttonCode)
{
	assert(buttonCode >= 0 || buttonCode <= 6);
	buttonStates[buttonCode] = false;
	if(currentHoldObjects[buttonCode])
	{
		m_TerminateHoldObject(buttonCode);
	}
}
ScoreHitRating Scoring::GetScoreHitRatingFromMs(int64 delta)
{
	delta = abs(delta);
	if(delta < maxEarlyHitTime)
	{
		if(delta < perfectHitTime)
			return ScoreHitRating::Perfect;
		return ScoreHitRating::Good;
	}
	return ScoreHitRating::Miss;
}
int64 Scoring::GetObjectHitDelta(ObjectState* state, uint32 buttonCode)
{
	if(!recentHits.Contains(state))
		return 0;
	return recentHits[state].hitDeltas[buttonCode];
}
int64 Scoring::GetAverageHitDelta() const
{
	if(m_numNotesHit == 0)
		return 0;
	return (int64)((double)m_hitNotesDelta / (double)m_numNotesHit);
}

void Scoring::m_AddScore(ObjectState* state, uint32 buttonCode, int64 delta)
{
	ScoreHitRating hitRating = GetScoreHitRatingFromMs(delta);

	if(hitRating == ScoreHitRating::Miss)
		currentComboCounter = 0;
	else
		currentComboCounter++;

	currentMaxScore += (size_t)ScoreHitRating::Perfect;
	currentHitScore += (size_t)hitRating;
	recentHits[state].hitDeltas[buttonCode] = delta;
}
void Scoring::m_TerminateHoldObject(uint32 buttonCode)
{
	ButtonState& btn = currentHoldObjects[buttonCode]->buttons[buttonCode];
	if(m_RecordHit(currentHoldObjects[buttonCode], buttonCode))
	{
		float holdRate = (float)holdDurations[buttonCode] / (float)btn.duration;
		Logf("Hold button [%i] released, score: %f", Logger::Info, buttonCode, holdRate);

		int64 timeBeforeEnd = (int64)(m_currentTime) - (int64)(currentHoldObjects[buttonCode]->time + btn.duration);
		if(timeBeforeEnd < -maxEarlyHitTime)
		{
			m_AddScore(currentHoldObjects[buttonCode], buttonCode, holdStartDelta[buttonCode]);

			// This automatically calculates a miss using timeBeforeEnd as a delta
			m_AddScore(currentHoldObjects[buttonCode], buttonCode, timeBeforeEnd);
			// Register miss
			OnButtonHit.Call(currentHoldObjects[buttonCode], buttonCode);
		}
		else
		{
			m_AddScore(currentHoldObjects[buttonCode], buttonCode, holdStartDelta[buttonCode]);
		}
	}

	// Clear hold object
	currentHoldObjects[buttonCode] = nullptr;
}
bool Scoring::m_RecordHit(ObjectState* state, uint32 buttonCode)
{
	ObjectHitState& setting = recentHits.FindOrAdd(state);
	uint32 mask = 1 << buttonCode;
	if((setting.mask & mask) != mask)
	{
		setting.mask |= mask;

		return true;
	}
	return false;
}

