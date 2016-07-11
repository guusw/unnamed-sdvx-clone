#include "stdafx.h"
#include "Scoring.hpp"
#include "BeatmapPlayback.hpp"
#include <math.h>

const MapTime Scoring::goodHitTime = 80;
const MapTime Scoring::perfectHitTime = 40;
const float Scoring::idleLaserSpeed = 0.5f;

Scoring::Scoring()
{
}
Scoring::~Scoring()
{
	m_CleanupHitStats();
	m_CleanupTicks();
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

void Scoring::SetInput(Input* input)
{
	m_CleanupInput();
	if(input)
	{
		m_input = input;
		m_input->OnButtonPressed.Add(this, &Scoring::m_OnButtonPressed);
		m_input->OnButtonReleased.Add(this, &Scoring::m_OnButtonReleased);
	}
}
void Scoring::m_CleanupInput()
{
	if(m_input)
	{
		m_input->OnButtonPressed.RemoveAll(this);
		m_input->OnButtonReleased.RemoveAll(this);
		m_input = nullptr;
	}
}

void Scoring::Reset()
{
	// Reset score/combo counters
	currentMaxScore = 0;
	currentHitScore = 0;
	currentComboCounter = 0;

	// Reset laser positions
	laserTargetPositions[0] = 0.0f;
	laserTargetPositions[1] = 0.0f;
	laserPositions[0] = 0.0f;
	laserPositions[1] = 1.0f;

	// Clear hit statistics
	hitStats.clear();

	// Recalculate maximum score
	totalMaxScore = CalculateMaxScore();

	// Recalculate gauge gain
	criticalGaugeGain = 3.5f / totalMaxScore;
	currentGauge = 0.0f;

	m_CleanupHitStats();
	m_CleanupTicks();
}

void Scoring::Tick(float deltaTime)
{
	m_UpdateLasers(deltaTime);
	m_UpdateTicks();
}

float Scoring::GetLaserRollOutput(uint32 index)
{
	assert(index >= 0 && index <= 1);
	if(m_holdObjects[6+index])
	{
		if(index == 0)
			return -laserTargetPositions[index];
		if(index == 1)
			return (1.0f - laserTargetPositions[index]);
	}
	return 0.0f;
}

static const float laserOutputInterpolationDuration = 0.1f;
float Scoring::GetLaserOutput()
{
	float f = Math::Min(1.0f, m_timeSinceOutputSet / laserOutputInterpolationDuration);
	return m_laserOutputSource + (m_laserOutputTarget - m_laserOutputSource) * f;
}
float Scoring::m_GetLaserOutputRaw()
{
	float val = 0.0f;
	for(int32 i = 0; i < 2; i++)
	{
		if(IsLaserHeld(i) && m_currentLaserSegments[i])
		{
			float actual = laserTargetPositions[i];
			// Undo laser extension
			if((m_currentLaserSegments[i]->flags & LaserObjectState::flag_Extended) != 0)
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
void Scoring::m_UpdateLaserOutput(float deltaTime)
{
	m_timeSinceOutputSet += deltaTime;
	float v = m_GetLaserOutputRaw();
	if(v != m_laserOutputTarget)
	{
		m_laserOutputTarget = v;
		m_laserOutputSource = GetLaserOutput();
		m_timeSinceOutputSet = m_interpolateLaserOutput ? 0.0f : laserOutputInterpolationDuration;
	}
}

HitStat* Scoring::m_AddOrUpdateHitStat(ObjectState* object)
{
	if(object->type == ObjectType::Single)
	{
		HitStat* stat = new HitStat(object);
		hitStats.Add(stat);
		return stat;
	}
	else if(object->type == ObjectType::Hold)
	{
		HoldObjectState* hold = (HoldObjectState*)object;
		HitStat** foundStat = m_holdHitStats.Find(object);
		if(foundStat)
			return *foundStat;
		HitStat* stat = new HitStat(object);
		hitStats.Add(stat);
		m_holdHitStats.Add(object, stat);

		// Get tick count
		Vector<MapTime> ticks;
		m_CalculateHoldTicks(hold, ticks);
		stat->holdMax = (uint32)ticks.size();

		return stat;
	}
	else if(object->type == ObjectType::Laser)
	{
		LaserObjectState* rootLaser = ((LaserObjectState*)object)->GetRoot();
		HitStat** foundStat = m_holdHitStats.Find(*rootLaser);
		if(foundStat)
			return *foundStat;
		HitStat* stat = new HitStat(*rootLaser);
		hitStats.Add(stat);
		m_holdHitStats.Add(object, stat);

		// Get tick count
		Vector<ScoreTick> ticks;
		m_CalculateLaserTicks(rootLaser, ticks);
		stat->holdMax = (uint32)ticks.size();

		return stat;
	}

	// Shouldn't get here
	assert(false);
	return nullptr;
}

void Scoring::m_CleanupHitStats()
{
	for(HitStat* hit : hitStats)
		delete hit;
	hitStats.clear();
	m_holdHitStats.clear();
}

bool Scoring::IsObjectHeld(ObjectState* object)
{
	if(object->type == ObjectType::Laser)
	{
		// Select root node of laser
		object = *((LaserObjectState*)object)->GetRoot();
	}

	return m_heldObjects.Contains(object);
}
bool Scoring::IsObjectHeld(uint32 index) const
{
	assert(index < 8);
	return m_holdObjects[index] != nullptr;
}
bool Scoring::IsLaserHeld(uint32 laserIndex, bool includeSlams) const
{
	if(includeSlams)
		return IsObjectHeld(laserIndex + 6);

	if(m_holdObjects[laserIndex+6])
	{
		// Check for slams
		return (((LaserObjectState*)m_holdObjects[laserIndex + 6])->flags & LaserObjectState::flag_Instant) == 0;
	}
	return false;
}

bool Scoring::IsLaserIdle(uint32 index) const
{
	return m_laserSegmentQueue.empty() && m_currentLaserSegments[0] == nullptr && m_currentLaserSegments[1] == nullptr;
}

void Scoring::m_CalculateHoldTicks(HoldObjectState* hold, Vector<MapTime>& ticks) const
{
	const TimingPoint* tp = m_playback->GetTimingPointAt(hold->time);
	// Get the amount of ticks in this hold note by taking the length 
	// ticks at 1/16
	const double tickRate = (tp->GetBPM() >= 250) ? 8 : 16;
	double tickInterval = Math::Floor(tp->GetBarDuration() / tickRate);
	uint32 numTicks = (uint32)Math::Floor((double)hold->duration / tickInterval);
	if(numTicks < 1)
		numTicks = 1; // At least 1 tick at the start

	for(uint32 i = 0; i < numTicks; i++)
	{
		ticks.Add((MapTime)((double)hold->time + tickInterval * (double)i));
	}
}
void Scoring::m_CalculateLaserTicks(LaserObjectState* laserRoot, Vector<ScoreTick>& ticks) const
{
	assert(laserRoot->prev == nullptr);
	const TimingPoint* tp = m_playback->GetTimingPointAt(laserRoot->time);
	// Get the amount of ticks in this laser by taking the length 
	// ticks at 1/16
	const double tickRate = (tp->GetBPM() >= 250) ? 8 : 16;
	double tickInterval = Math::Floor(tp->GetBarDuration() / 16.0);

	LaserObjectState* sectionStart = laserRoot;
	MapTime sectionStartTime = laserRoot->time;
	MapTime combinedDuration = 0;
	//float lastDelta = laserRoot->points[1] - laserRoot->points[1];
	LaserObjectState* lastSlam = nullptr;
	auto AddTicks = [&]()
	{
		uint32 numTicks = (uint32)Math::Floor((double)combinedDuration / tickInterval);
		for(uint32 i = 0; i < numTicks; i++)
		{
			if(lastSlam && i == 0) // No first tick if connected to slam
				continue;

			ScoreTick& t = ticks.Add(ScoreTick(*sectionStart));
			t.time = sectionStartTime + (MapTime)(tickInterval*(double)i);
			t.flags = TickFlags::Laser;
			
			// Link this tick to the correct segment
			if(sectionStart->next && (sectionStart->time + sectionStart->duration) <= t.time)
			{
				assert((sectionStart->next->flags & LaserObjectState::flag_Instant) == 0);
				t.object = *(sectionStart = sectionStart->next);
			}


			if(!lastSlam && i == 0)
				t.SetFlag(TickFlags::Start);
		}
		combinedDuration = 0;
	};

	for(auto it = laserRoot; it; it = it->next)
	{
		if((it->flags & LaserObjectState::flag_Instant) != 0)
		{
			AddTicks();
			ScoreTick& t = ticks.Add(ScoreTick(*it));
			t.time = it->time;
			t.flags = TickFlags::Laser | TickFlags::Slam;
			lastSlam = it;
			if(it->next)
			{
				sectionStart = it->next;
				sectionStartTime = it->next->time;
			}
			else
			{
				sectionStart = nullptr;
				sectionStartTime = it->time;
			}		  
		}
		else
		{
			/// TODO
			// New tick sequence when switching directions?
			//float currentDelta = it->points[1] - it->points[0];
			//if(Math::Sign(currentDelta) != Math::Sign(lastDelta)) 
			//{
			//
			//}

			combinedDuration += it->duration;
		}
	}
	AddTicks();
	if(ticks.size() > 0)
		ticks.back().SetFlag(TickFlags::End);
}

void Scoring::m_OnObjectEntered(ObjectState* obj)
{
	// The following code registers which ticks exist depending on the object type / duration
	if(obj->type == ObjectType::Single)
	{
		ButtonObjectState* bt = (ButtonObjectState*)obj;
		ScoreTick* t = m_ticks[bt->index].Add(new ScoreTick(obj));
		t->time = bt->time;
		t->SetFlag(TickFlags::Button);

	}
	else if(obj->type == ObjectType::Hold)
	{
		const TimingPoint* tp = m_playback->GetTimingPointAt(obj->time);
		HoldObjectState* hold = (HoldObjectState*)obj;
		
		// Add all hold ticks
		Vector<MapTime> holdTicks;
		m_CalculateHoldTicks(hold, holdTicks);
		for(size_t i = 0; i < holdTicks.size(); i++)
		{
			ScoreTick* t = m_ticks[hold->index].Add(new ScoreTick(obj));
			t->SetFlag(TickFlags::Hold);
			if(i == 0)
				t->SetFlag(TickFlags::Start);
			if(i == holdTicks.size() - 1)
				t->SetFlag(TickFlags::End);
			t->time = holdTicks[i];
		}
	}
	else if(obj->type == ObjectType::Laser)
	{
		LaserObjectState* laser = (LaserObjectState*)obj;
		if(!laser->prev) // Only register root laser objects
		{
			// All laser ticks, including slam segments
			Vector<ScoreTick> laserTicks;
			m_CalculateLaserTicks(laser, laserTicks);
			for(size_t i = 0; i < laserTicks.size(); i++)
			{
				// Add copy
				m_ticks[laser->index + 6].Add(new ScoreTick(laserTicks[i]));
			}
		}

		// Add to laser segment queue
		m_laserSegmentQueue.Add(laser);
	}
}
void Scoring::m_OnObjectLeaved(ObjectState* obj)
{
	if(obj->type == ObjectType::Laser)
	{
		LaserObjectState* laser = (LaserObjectState*)obj;
		if(laser->next != nullptr)
			return; // Only terminate holds on last of laser section
		obj = *laser->GetRoot();
	}
	m_ReleaseHoldObject(obj);
}

void Scoring::m_UpdateTicks()
{
	MapTime currentTime = m_playback->GetLastTime();

	// This loop checks for ticks that are missed
	for(uint32 buttonCode = 0; buttonCode < 8; buttonCode++)
	{
		Input::Button button = (Input::Button)buttonCode;

		// List of ticks for the current button code
		auto& ticks = m_ticks[buttonCode];
		for(uint32 i = 0; i < ticks.size(); i++)
		{
			ScoreTick* tick = ticks[i];
			MapTime delta = currentTime - ticks[i]->time;
			bool shouldMiss = delta > tick->GetHitWindow();
			bool processed = false;
			if(delta >= 0)
			{
				if(tick->HasFlag(TickFlags::Button) && autoplay)
				{
					m_TickHit(tick, buttonCode, 0);
					processed = true;
				}

				if(tick->HasFlag(TickFlags::Hold))
				{
					// Ignore the first hold note ticks
					//	except for autoplay, which just hits it.
					if(!tick->HasFlag(TickFlags::Start) || autoplay)
					{
						// Check buttons here for holds
						if(m_input && (m_input->GetButton(button) || autoplay))
						{
							m_TickHit(tick, buttonCode);
							processed = true;
						}
					}
				}
				else if(tick->HasFlag(TickFlags::Laser))
				{
					LaserObjectState* laserObject = (LaserObjectState*)tick->object;
					if(tick->HasFlag(TickFlags::Slam))
					{
						// Check if slam hit
						float dirSign = Math::Sign(laserObject->GetDirection());
						float inputSign = Math::Sign(m_input->GetInputLaserDir(buttonCode - 6));
						if(autoplay)
							inputSign = dirSign;
						if(dirSign == inputSign && delta > 0)
						{
							m_TickHit(tick, buttonCode);
							processed = true;
						}
					}
					else
					{
						// Check laser input
						float laserDelta = abs(laserPositions[laserObject->index] - laserTargetPositions[laserObject->index]);
						if(laserDelta < 0.083f)
						{
							m_TickHit(tick, buttonCode);
							processed = true;
						}
					}
				}

				if(shouldMiss && !processed)
				{
					m_TickMiss(tick, buttonCode, delta);
					processed = true;
				}

				if(processed)
				{
					delete tick;
					ticks.Remove(tick, false);
					i--;
				}
				else
				{
					// No further ticks to process
					break;
				}
			}
		}
	}
}
ObjectState* Scoring::m_ConsumeTick(uint32 buttonCode)
{
	MapTime currentTime = m_playback->GetLastTime();

	assert(buttonCode < 8);
	auto& ticks = m_ticks[buttonCode];
	for(uint32 i = 0; i < ticks.size(); i++)
	{
		ScoreTick* tick = ticks[i];
		MapTime delta = currentTime - ticks[i]->time;
		if(abs(delta) < tick->GetHitWindow())
		{
			ObjectState* hitObject = tick->object;

			if(tick->HasFlag(TickFlags::Laser))
			{
				// Ignore laser ticks
				continue;
			}

			m_TickHit(tick, buttonCode, delta);
			delete tick;
			ticks.Remove(tick, false);

			return hitObject;
		}
	}
	return nullptr;
}

void Scoring::m_OnTickProcessed(ScoreTick* tick, uint32 index)
{
}
void Scoring::m_TickHit(ScoreTick* tick, uint32 index, MapTime delta /*= 0*/)
{
	HitStat* stat = m_AddOrUpdateHitStat(tick->object);
	if(tick->HasFlag(TickFlags::Button))
	{
		stat->delta = delta;
		stat->rating = tick->GetHitRatingFromDelta(delta);
		OnButtonHit.Call((Input::Button)index, stat->rating, tick->object);
		m_AddScore((uint32)stat->rating);
	}
	else if(tick->HasFlag(TickFlags::Hold))
	{
		m_SetHoldObject(tick->object, index);

		stat->rating = ScoreHitRating::Perfect;
		stat->hold++;

		m_AddScore(2);
	}
	else if(tick->HasFlag(TickFlags::Laser))
	{
		LaserObjectState* rootObject = ((LaserObjectState*)tick->object)->GetRoot();
		if(tick->HasFlag(TickFlags::Slam))
		{
			OnLaserSlamHit.Call((LaserObjectState*)tick->object);
		}
		m_SetHoldObject(*rootObject, index);
		m_AddScore(2);

		stat->rating = ScoreHitRating::Perfect;
		stat->hold++;
	}
	m_OnTickProcessed(tick, index);
}
void Scoring::m_TickMiss(ScoreTick* tick, uint32 index, MapTime delta)
{
	HitStat* stat = m_AddOrUpdateHitStat(tick->object);
	if(tick->HasFlag(TickFlags::Button))
	{
		OnButtonMiss.Call((Input::Button)index); 
		stat->rating = ScoreHitRating::Miss;
		stat->delta = delta;
		currentGauge -= 0.02f;
	}
	else if(tick->HasFlag(TickFlags::Hold))
	{
		m_ReleaseHoldObject(index);
		currentGauge -= 0.005f;
		stat->rating = ScoreHitRating::Miss;
	}
	else if(tick->HasFlag(TickFlags::Laser))
	{
		m_ReleaseHoldObject(index);
		currentGauge -= 0.005f;
		stat->rating = ScoreHitRating::Miss;
	}
	// All misses reset combo
	currentGauge = std::max(0.0f, currentGauge);
	m_ResetCombo();
	m_OnTickProcessed(tick, index);
}

void Scoring::m_CleanupTicks()
{
	for(uint32 i = 0; i < 8; i++)
	{
		for(ScoreTick* tick : m_ticks[i])
			delete tick;
		m_ticks[i].clear();
	}
}

void Scoring::m_AddScore(uint32 score)
{
	assert(score > 0 && score <= 2);
	currentHitScore += score;
	if (score == 2)
	{
		currentGauge += criticalGaugeGain;
	}
	else
	{
		currentGauge += criticalGaugeGain / 3.0f;
	}
	currentGauge = std::min(1.0f, currentGauge);
	currentComboCounter += 1;
	OnComboChanged.Call(currentComboCounter);
}
void Scoring::m_ResetCombo()
{
	currentComboCounter = 0;
	OnComboChanged.Call(currentComboCounter);
}

void Scoring::m_SetHoldObject(ObjectState* obj, uint32 index)
{
	if(m_holdObjects[index] != obj)
	{
		assert(!m_heldObjects.Contains(obj));
		m_heldObjects.Add(obj);
		m_holdObjects[index] = obj;
		OnObjectHold.Call((Input::Button)index, obj);
	}
}
void Scoring::m_ReleaseHoldObject(ObjectState* obj)
{
	auto it = m_heldObjects.find(obj);
	if(it != m_heldObjects.end())
	{
		m_heldObjects.erase(it);

		// Unset hold objects
		for(uint32 i = 0; i < 8; i++)
		{
			if(m_holdObjects[i] == obj)
			{
				m_holdObjects[i] = nullptr;
				OnObjectReleased.Call((Input::Button)i, obj);
				return;
			}
		}

		// Should never get here
		assert(false);
	}
}
void Scoring::m_ReleaseHoldObject(uint32 index)
{
	m_ReleaseHoldObject(m_holdObjects[index]);
}

void Scoring::m_UpdateLasers(float deltaTime)
{
	MapTime mapTime = m_playback->GetLastTime();
	for(uint32 i = 0; i < 2; i++)
	{
		// Check for new laser segments in laser queue
		for(auto it = m_laserSegmentQueue.begin(); it != m_laserSegmentQueue.end();)
		{
			if((*it)->time >= mapTime)
			{
				// Replace the currently active segment
				m_currentLaserSegments[(*it)->index] = *it;
				it = m_laserSegmentQueue.erase(it);
				continue;
			}
			it++;
		}
		
		LaserObjectState* currentSegment = m_currentLaserSegments[i];
		if(currentSegment)
		{
			if((currentSegment->time + currentSegment->duration) < mapTime)
			{
				currentSegment = nullptr;
				m_currentLaserSegments[i] = nullptr;
			}
			else
			{
				// Update target position
				laserTargetPositions[i] = currentSegment->SamplePosition(mapTime);
			}
		}

		m_laserInput[i] = autoplay ? 0.0f : m_input->GetInputLaserDir(i);

		// Update idle laser
		if(IsLaserIdle(i))
		{
			laserPositions[i] = Math::Clamp(laserPositions[i] + m_laserInput[i] * deltaTime * idleLaserSpeed, 0.0f, 1.0f);

			if(m_laserInput[i] == 0.0f)
			{
				m_timeSinceLaserInput[i] += deltaTime;
				// Slowly move laser towards base when not being used
				if(m_timeSinceLaserInput[i] > 2.0f)
				{
					static const float targets[2] = { -1.0f, 1.0f };
					laserPositions[i] = Math::Clamp(laserPositions[i] + targets[i] * deltaTime * idleLaserSpeed, 0.0f, 1.0f);
				}
			}
			else
			{
				m_timeSinceLaserInput[i] = 0.0f;
			}
		}
		else
		{
			if(currentSegment)
			{
				// Update laser gameplay
				float positionDelta = laserTargetPositions[i] - laserPositions[i];
				float laserDir = currentSegment->GetDirection();
				float moveDir = Math::Sign(positionDelta);
				float input = m_input->GetInputLaserDir(i);
				if(autoplay)
				{
					if(abs(positionDelta) > 0.05f)
						input = positionDelta;
					else
						input = laserDir;
				}
				float inputDir = Math::Sign(input);

				if(inputDir != 0.0f)
				{
					// Snap to laser if laser is on the wrong side
					if(moveDir != laserDir && inputDir == laserDir)
					{
						laserPositions[i] = laserTargetPositions[i];
					}
					else if(moveDir == inputDir)
					{
						moveDir *= abs(input);
						if(moveDir < 0)
						{
							laserPositions[i] = Math::Max(laserPositions[i] + moveDir, laserTargetPositions[i]);
						}
						else
						{
							laserPositions[i] = Math::Min(laserPositions[i] + moveDir, laserTargetPositions[i]);
						}
					}
				}
			}
			m_timeSinceLaserInput[i] = 0.0f;
		}
	}

	// Interpolate laser output
	m_UpdateLaserOutput(deltaTime);
}

void Scoring::m_OnButtonPressed(Input::Button buttonCode)
{
	// Ignore buttons on autoplay
	if(autoplay)
		return;

	if(buttonCode < Input::Button::LS_0Neg)
	{
		ObjectState* obj = m_ConsumeTick((uint32)buttonCode);
		if(!obj)
		{
			// Fire event for idle hits
			OnButtonHit.Call(buttonCode, ScoreHitRating::Idle, nullptr);
		}
	}
	else
	{
		ObjectState* obj = nullptr;
		if(buttonCode < Input::Button::LS_1Neg)
			obj = m_ConsumeTick(6); // Laser L
		else
			obj = m_ConsumeTick(7); // Laser R
	}
}
void Scoring::m_OnButtonReleased(Input::Button buttonCode)
{
}

uint32 Scoring::CalculateMaxScore() const
{
	const Beatmap& map = m_playback->GetBeatmap();

	Set<LaserObjectState*> processedLasers;

	assert(m_playback);
	auto& objects = map.GetLinearObjects();
	uint32 maxScore = 0;
	for(auto& _obj : objects)
	{
		MultiObjectState* obj = *_obj;
		const TimingPoint* tp = m_playback->GetTimingPointAt(obj->time);
		if(obj->type == ObjectType::Single)
		{
			maxScore += (uint32)ScoreHitRating::Perfect;
		}
		else if(obj->type == ObjectType::Hold)
		{
			Vector<MapTime> holdTicks;
			m_CalculateHoldTicks((HoldObjectState*)obj, holdTicks);
			maxScore += (uint32)ScoreHitRating::Perfect * (uint32)holdTicks.size();
		}
		else if(obj->type == ObjectType::Laser)
		{
			LaserObjectState* laserRoot = obj->laser.GetRoot();

			// Don't evaluate ticks for every segment, only for entire chains of segments
			if(!processedLasers.Contains(laserRoot))
			{
				Vector<ScoreTick> laserTicks;
				m_CalculateLaserTicks((LaserObjectState*)obj, laserTicks);
				maxScore += (uint32)ScoreHitRating::Perfect * (uint32)laserTicks.size();
				processedLasers.Add(laserRoot);
			}
		}
	}
	return maxScore;
}

uint32 Scoring::CalculateCurrentScore() const
{
	// Final Score = Score * 0.9 + Gauge * 0.1
	return (uint32)((double)currentHitScore / (double)totalMaxScore * 900000 +
		currentGauge * 100000.0);
}

MapTime ScoreTick::GetHitWindow() const
{
	// Hold ticks don't have a hit window, but the first ones do
	if(HasFlag(TickFlags::Hold) && !HasFlag(TickFlags::Start))
		return 0;
	// Laser ticks also don't have a hit window except for the first ticks and slam segments
	if(HasFlag(TickFlags::Laser))
	{
		if(!HasFlag(TickFlags::Start) && !HasFlag(TickFlags::Slam))
			return 0;
	}
	return Scoring::goodHitTime;
}
ScoreHitRating ScoreTick::GetHitRating(MapTime currentTime) const
{
	MapTime delta = abs(time - currentTime);
	return GetHitRatingFromDelta(delta);
}
ScoreHitRating ScoreTick::GetHitRatingFromDelta(MapTime delta) const
{
	if(HasFlag(TickFlags::Button))
	{
		// Button hit judgeing
		if(delta > GetHitWindow())
			return ScoreHitRating::Miss;
		if(delta < Scoring::perfectHitTime)
			return ScoreHitRating::Perfect;
		return ScoreHitRating::Good;
	}
	return ScoreHitRating::Perfect;
}

bool ScoreTick::HasFlag(TickFlags flag) const
{
	return (flags & flag) != TickFlags::None;
}
void ScoreTick::SetFlag(TickFlags flag)
{
	flags = flags | flag;
}
TickFlags operator|(const TickFlags& a, const TickFlags& b)
{
	return (TickFlags)((uint8)a | (uint8)b);
}
TickFlags operator&(const TickFlags& a, const TickFlags& b)
{
	return (TickFlags)((uint8)a & (uint8)b);
}
