#pragma once
#include "Beatmap.hpp"

/*
Manages the iteration over beatmaps
*/
class BeatmapPlayback
{
public:
	BeatmapPlayback() = default;
	BeatmapPlayback(Beatmap& beatmap);

	// Resets the playback of the map
	// be sure to also reset the input timing whatever the source may be
	void Reset();

	// Updates the time of the playback
	// checks all items that have been triggered between last time and this time
	// if it is a new timing point, this is used for the new BPM
	void Update(MapTime newTime);

	// Gets all linear objects that fall within the given range
	// range is in bars
	Vector<ObjectState*> GetObjectsInRange(float range);
	Vector<ObjectState*> GetObjectsInRange(MapTime range);

	const TimingPoint& GetCurrentTimingPoint() const;

	uint32 CountBeats(MapTime start, MapTime range, uint32& startIndex, uint32 multiplier = 1) const;

	MapTime BarDistanceToTime(float distance);
	float TimeToBarDistance(MapTime time);
	float DurationToBarDistance(MapTime duration);

	// Value from 0 to 1 that indicates how far in a single bar the playback is
	float GetBarTime() const;

	// Current map time in ms as last passed to Update
	MapTime GetLastTime() const;

	// Float value of how many bars have passed
	float GetTotalBarTime() const;

	// Check if a laser segment is instant
	bool IsLaserSlam(const LaserState& s, MapTime time);

	// Duration for objects to keep being returned by GetObjectsInRange after they have passed
	MapTime keepObjectDuration = 1000;

private:
	TimingPoint** m_currentTiming = nullptr;
	ObjectState** m_currentObj = nullptr;
	bool m_IsLastTimingPoint() const;
	bool m_IsLastObject() const;

	MapTime m_lastTime;
	Vector<TimingPoint*> m_timingPoints;
	Vector<ObjectState*> m_objects;
	Set<ObjectState*> m_holdObjects;

	float m_barTime;
	uint32 m_totalBars;

	Beatmap* m_beatmap = nullptr;
};