#include "stdafx.h"
#include "BeatmapPlayback.hpp"
#include "Profiling.hpp"

BeatmapPlayback::BeatmapPlayback(Beatmap& beatmap) : m_beatmap(&beatmap)
{
	m_timingPoints = m_beatmap->GetLinearTimingPoints();
	m_objects = m_beatmap->GetLinearObjects();
	Reset();
}
void BeatmapPlayback::Reset()
{
	assert(m_objects.size() > 0);
	assert(m_timingPoints.size() > 0);

	m_lastTime = 0;
	m_currentObj = &m_objects.front();
	m_currentTiming = &m_timingPoints.front();
}
void BeatmapPlayback::Update(MapTime newTime)
{
	MapTime delta = newTime - m_lastTime;
	if(newTime < m_lastTime)
	{
		Logf("New time was before last time %ull -> %ull", Logger::Warning, m_lastTime, newTime);
		return;
	}

	// Check passed hold object
	for(auto it = m_holdObjects.begin(); it != m_holdObjects.end();)
	{
		if((*it)->time + (*it)->maxDuration < newTime)
		{
			// It has passed
			it = m_holdObjects.erase(it);
			continue;
		}
		it++;
	}

	// Advance timing
	if(m_currentTiming)
	{
		while(!m_IsLastTimingPoint() && m_currentTiming[1]->offset <= newTime)
		{
			m_currentTiming += 1;
		}

		uint32 beatID = 0;
		uint32 nBeats = CountBeats(m_lastTime, delta, beatID);
		//for(uint32 i = 0; i < nBeats; i++)
		//	Logf("Beat %d/%d", Logger::Info, (beatID % m_currentTiming[0]->measure) + i, m_currentTiming[0]->measure);

		const TimingPoint& tp = GetCurrentTimingPoint();
		double effectiveTime = ((double)newTime - tp.offset); // Time with offset applied
		m_barTime = (float)fmod(effectiveTime / (tp.beatDuration * tp.measure), 1.0);
	}

	// Advance object
	if(m_currentObj)
	{
		while(!m_IsLastObject() && ((m_currentObj[1]->time + keepObjectDuration) <= newTime))
		{
			// Store hold objects for later
			if(m_currentObj[0]->maxDuration > 0)
			{
				m_holdObjects.insert(m_currentObj[0]);
			}
			m_currentObj += 1;
		}
	}

	m_lastTime = newTime;
}
Vector<ObjectState*> BeatmapPlayback::GetObjectsInRange(float range)
{
	return GetObjectsInRange(BarDistanceToTime(range));
}

Vector<ObjectState*> BeatmapPlayback::GetObjectsInRange(MapTime range)
{
	const TimingPoint& tp = GetCurrentTimingPoint();
	MapTime end = m_lastTime + range;
	Vector<ObjectState*> ret;

	// Always add hold objects
	for(auto& ho : m_holdObjects)
	{
		ret.Add(ho);
	}

	ObjectState** obj = m_currentObj;
	while(true)
	{
		if((*obj)->time > end)
			break; // No more objects

		ret.Add(*obj);

		if(obj == &m_objects.back())
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
uint32 BeatmapPlayback::CountBeats(MapTime start, MapTime range, uint32& startIndex, uint32 multiplier) const
{
	const TimingPoint& tp = GetCurrentTimingPoint();
	int64 delta = (int64)start - (int64)tp.offset;
	int64 beatStart = (int64)floor((double)delta / (tp.beatDuration/ multiplier));
	int64 beatEnd = (int64)floor((double)(delta + range) / (tp.beatDuration/ multiplier));
	startIndex = (uint32)beatStart + 1;
	return (uint32)(beatEnd - beatStart);
}

MapTime BeatmapPlayback::BarDistanceToTime(float distance)
{
	const TimingPoint& tp = GetCurrentTimingPoint();
	return (MapTime)(distance * tp.beatDuration * tp.measure);
}

float BeatmapPlayback::TimeToBarDistance(MapTime time)
{
	const TimingPoint& tp = GetCurrentTimingPoint();
	int64 delta = time - m_lastTime;
	return (float)((double)delta / (tp.beatDuration * tp.measure));
}
float BeatmapPlayback::DurationToBarDistance(MapTime duration)
{
	const TimingPoint& tp = GetCurrentTimingPoint();
	return (float)((double)duration / (tp.beatDuration * tp.measure));
}
float BeatmapPlayback::GetBarTime() const
{
	return m_barTime;
}
MapTime BeatmapPlayback::GetLastTime() const
{
	return m_lastTime;
}
float BeatmapPlayback::GetTotalBarTime() const
{
	return (float)m_totalBars + GetBarTime();
}

bool BeatmapPlayback::IsLaserSlam(const LaserState& s, MapTime time)
{
	if(s.duration > GetCurrentTimingPoint().GetSlamTreshold())
		return false;
	if(fabs(s.points[1] - s.points[0]) < 0.05f)
		return false;
	return true;
}

bool BeatmapPlayback::m_IsLastTimingPoint() const
{
	return m_currentTiming == &m_timingPoints.back();
}
bool BeatmapPlayback::m_IsLastObject() const
{
	return m_currentObj == &m_objects.back();
}
