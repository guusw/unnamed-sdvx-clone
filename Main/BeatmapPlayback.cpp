#include "stdafx.h"
#include "BeatmapPlayback.hpp"
#include "Profiling.hpp"

BeatmapPlayback::BeatmapPlayback(Beatmap& beatmap) : m_beatmap(&beatmap)
{
}
bool BeatmapPlayback::Reset()
{
	m_timingPoints = m_beatmap->GetLinearTimingPoints();
	m_objects = m_beatmap->GetLinearObjects();
	if(m_objects.size() == 0)
		return false;
	if(m_timingPoints.size() == 0)
		return false;

	m_playbackTime = 0;
	m_currentObj = &m_objects.front();
	m_currentTiming = &m_timingPoints.front();

	m_barTime = 0;
	return true;
}

void BeatmapPlayback::Update(MapTime newTime)
{
	MapTime delta = newTime - m_playbackTime;
	if(newTime < m_playbackTime)
	{
		// Don't allow backtracking
		Logf("New time was before last time %ull -> %ull", Logger::Warning, m_playbackTime, newTime);
		return;
	}

	// Count bars
	uint32 beatID = 0;
	uint32 nBeats = CountBeats(m_playbackTime - delta, delta, beatID);
	const TimingPoint& tp = GetCurrentTimingPoint();
	double effectiveTime = ((double)newTime - tp.time); // Time with offset applied
	m_barTime = (float)fmod(effectiveTime / (tp.beatDuration * tp.measure), 1.0);

	// Set new time
	m_playbackTime = newTime;

	// Advance timing
	TimingPoint** newTimingPoint = m_SelectTimingPoint(m_playbackTime);
	if(newTimingPoint != m_currentTiming)
	{
		m_currentTiming = newTimingPoint;
	}

	// Advance objects
	ObjectState** newObject = m_SelectHitObject(m_playbackTime+hittableObjectTreshold);
	if(newObject != m_currentObj)
	{
		// Iterate over objects that can be hit or have passed that
		for(ObjectState** it = m_currentObj; it != newObject; it++)
		{
			MultiObjectState* obj = **it;
			m_hittableObjects.Add(*it);
			OnObjectEntered.Call(*it);
		}
		m_currentObj = newObject;
	}

	// Check passed hittable objects
	MapTime objectPassTime = m_playbackTime - hittableObjectTreshold;
	for(auto it = m_hittableObjects.begin(); it != m_hittableObjects.end();)
	{
		MultiObjectState* obj = **it;
		if(obj->type == ObjectType::Hold)
		{
			if((obj->hold.duration + obj->time) < objectPassTime)
			{
				OnObjectLeaved.Call(*it);
				it = m_hittableObjects.erase(it);
				continue;
			}
		}
		else if(obj->type == ObjectType::Laser)
		{
			if((obj->laser.duration + obj->time) < objectPassTime)
			{
				OnObjectLeaved.Call(*it);
				it = m_hittableObjects.erase(it);
				continue;
			}
		}
		else if(obj->type == ObjectType::Single)
		{
			if(obj->time < objectPassTime)
			{
				OnObjectLeaved.Call(*it);
				it = m_hittableObjects.erase(it);
				continue;
			}
		}
		it++;
	}
}

Set<ObjectState*>& BeatmapPlayback::GetHittableObjects()
{
	return m_hittableObjects;
}
Vector<ObjectState*> BeatmapPlayback::GetObjectsInRange(MapTime range)
{
	const TimingPoint& tp = GetCurrentTimingPoint();
	MapTime end = m_playbackTime + range;
	Vector<ObjectState*> ret;

	// Always add hold objects
	for(auto& ho : m_hittableObjects)
	{
		ret.Add(ho);
	}

	// Return all objects that lie after the currently queued object and fall within the given range
	ObjectState** obj = m_currentObj;
	while(true)
	{
		if((*obj)->time > end)
			break; // No more objects

		ret.Add(*obj);

		if(IsLastObject(obj))
			break;
		obj += 1; // Next
	}

	return ret;
}

const TimingPoint& BeatmapPlayback::GetCurrentTimingPoint() const
{
	assert(m_currentTiming);
	return **m_currentTiming;
}
const TimingPoint* BeatmapPlayback::GetTimingPointAt(MapTime time) const
{
	return *const_cast<BeatmapPlayback*>(this)->m_SelectTimingPoint(time);
}

uint32 BeatmapPlayback::CountBeats(MapTime start, MapTime range, uint32& startIndex, uint32 multiplier) const
{
	const TimingPoint& tp = GetCurrentTimingPoint();
	int64 delta = (int64)start - (int64)tp.time;
	int64 beatStart = (int64)floor((double)delta / (tp.beatDuration/ multiplier));
	int64 beatEnd = (int64)floor((double)(delta + range) / (tp.beatDuration/ multiplier));
	startIndex = (uint32)beatStart + 1;
	return (uint32)(beatEnd - beatStart);
}
MapTime BeatmapPlayback::BarDistanceToDuration(float distance)
{
	const TimingPoint& tp = GetCurrentTimingPoint();
	return (MapTime)(distance * tp.beatDuration * tp.measure);
}
float BeatmapPlayback::DurationToBarDistance(MapTime duration)
{
	const TimingPoint& tp = GetCurrentTimingPoint();
	return (float)((double)duration / (tp.beatDuration * tp.measure));
}
float BeatmapPlayback::TimeToBarDistance(MapTime time)
{
	const TimingPoint& tp = GetCurrentTimingPoint();
	int64 delta = time - m_playbackTime;
	return (float)((double)delta / (tp.beatDuration * tp.measure));
}

float BeatmapPlayback::GetBarTime() const
{
	return m_barTime;
}
MapTime BeatmapPlayback::GetLastTime() const
{
	return m_playbackTime;
}
TimingPoint** BeatmapPlayback::m_SelectTimingPoint(MapTime time)
{
	TimingPoint** tpStart = m_currentTiming;

	// Start at front of array if current object lies ahead of given input time
	if(tpStart[0]->time > time)
		tpStart = &m_timingPoints.front();

	// Keep advancing the start pointer while the next object's starting time lies before the input time
	while(true)
	{
		if(!IsLastTiming(tpStart) && tpStart[1]->time < time)
			tpStart = tpStart + 1;
		else
			break;
	}

	return tpStart;
}
ObjectState** BeatmapPlayback::m_SelectHitObject(MapTime time)
{
	ObjectState** objStart = m_currentObj;

	// Start at front of array if current object lies ahead of given input time
	if(objStart[0]->time > time)
		objStart = &m_objects.front();

	// Keep advancing the start pointer while the next object's starting time lies before the input time
	while(true)
	{
		if(!IsLastObject(objStart) && objStart[1]->time < time)
			objStart = objStart + 1;
		else
			break;
	}

	return objStart;
}
bool BeatmapPlayback::IsLastTiming(TimingPoint** obj)
{
	return obj == &m_timingPoints.back();
}
bool BeatmapPlayback::IsLastObject(ObjectState** obj)
{
	return obj == &m_objects.back();
}
