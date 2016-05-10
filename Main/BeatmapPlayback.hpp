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
	// Must be called before any other function is called on this object
	// returns false if the map contains no objects or timing or otherwise invalid
	bool Reset();

	// Updates the time of the playback
	// checks all items that have been triggered between last time and this time
	// if it is a new timing point, this is used for the new BPM
	void Update(MapTime newTime);

	// Modifyable array of all hittable objects, within -+'hittableObjectTreshold' of current time
	Set<ObjectState*>& GetHittableObjects();
	MapTime hittableObjectTreshold = 100;

	// Gets all linear objects that fall within the given time range:
	//	<curr - keepObjectDuration, curr + range>
	Vector<ObjectState*> GetObjectsInRange(MapTime range);
	// Duration for objects to keep being returned by GetObjectsInRange after they have passed the current time
	MapTime keepObjectDuration = 1000;

	// Get the timing point at the current time
	const TimingPoint& GetCurrentTimingPoint() const;
	// Get the timing point at a given time
	const TimingPoint* GetTimingPointAt(MapTime time) const;
	
	// Counts the total amount of beats that have passed within <start, start+range>
	// Returns the number of passed beats
	// Returns the starting index of the passed beats in 'startIndex'
	// Additionally the time signature is multiplied by multiplier
	//	with a multiplier of 2 a 4/4 signature would tick twice as fast
	uint32 CountBeats(MapTime start, MapTime range, uint32& startIndex, uint32 multiplier = 1) const;

	// Bar calculations using the current timing point
	// A single bar is the duration of the amount of beats in the current time signature.
	MapTime BarDistanceToDuration(float distance);
	float DurationToBarDistance(MapTime time);
	float DurationToBarDistanceAtTime(MapTime time, MapTime duration);

	float TimeToBarDistance(MapTime time);

	// Current map time in ms as last passed to Update
	MapTime GetLastTime() const;

	// Value from 0 to 1 that indicates how far in a single bar the playback is
	float GetBarTime() const;

	// Gets the currently set value of a value set by events in the beatmap
	const EventData& GetEventData(EventKey key);
	// Retrieve event data as any 32-bit type
	template<typename T>
	const T& GetEventData(EventKey key)
	{
		assert(sizeof(T) <= 4);
		return *(T*)&GetEventData(key);
	}

	/* Playback events */
	// Called when an object became within the 'hittableObjectTreshold'
	Delegate<ObjectState*> OnObjectEntered;
	// Called after an object has passed the duration it can be hit in
	Delegate<ObjectState*> OnObjectLeaved;
	// Called when an FX button with effect enters
	Delegate<HoldObjectState*> OnFXBegin;
	// Called when an FX button with effect leaves
	Delegate<HoldObjectState*> OnFXEnd;

	Delegate<EventKey, EventData> OnEventChanged;

private:
	TimingPoint** m_currentTiming = nullptr;
	ObjectState** m_currentObj = nullptr;

	// Selects an object or timing point based on a given input state
	// if allowReset is true the search starts from the start of the object list if current point lies beyond given input time
	TimingPoint** m_SelectTimingPoint(MapTime time, bool allowReset = false);
	ObjectState** m_SelectHitObject(MapTime time, bool allowReset = false);

	// End object pointer, this is not a valid pointer, but points to the element after the last element
	bool IsEndTiming(TimingPoint** obj);
	bool IsEndObject(ObjectState** obj);

	// Current map position of this playback object
	MapTime m_playbackTime;
	Vector<TimingPoint*> m_timingPoints;
	Vector<ObjectState*> m_objects;

	// Contains all the objects that are in the current valid timing area
	Set<ObjectState*> m_hittableObjects;
	// Hold objects to render even when their start time is not in the current visibility range
	Set<ObjectState*> m_holdObjects;
	// Hold buttons with effects that are active
	Set<ObjectState*> m_effectObjects;

	// Current state of events
	Map<EventKey, EventData> m_eventMapping;

	float m_barTime;

	Beatmap* m_beatmap = nullptr;
};