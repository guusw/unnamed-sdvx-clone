#pragma once
#include "BeatmapPlayback.hpp"

// Hit rating for hitting a button
enum class ScoreHitRating
{
	Miss = 0,
	Good,
	Perfect,
};

// Recorded button hit state for a single ObjectState
struct ObjectHitState
{
	int64 hitDeltas[6];
	uint32 mask;
};
struct HitStat
{
	HitStat(MapTime time, MapTime delta) : time(time), delta(delta) {};
	bool operator<(const HitStat& other) { return time < other.time; }
	MapTime time;
	MapTime delta;
};

/*
	Calculates game score and checks which objects are hit
	also keeps track of laser positions
*/
class Scoring : public Unique
{
public:
	Scoring();

	// Needs to be set to find out which objects are active/hittable
	void SetPlayback(BeatmapPlayback& playback);

	// Resets/Initializes the scoring system
	// Called after SetPlayback
	void Reset();

	// Updates the list of objects that are possible to hit
	void Tick(float deltaTime);

	// Score for hiting an object with given delta
	ScoreHitRating GetHitRatingFromDelta(MapTime delta);
	// Delta of object as if it were hit right now
	MapTime GetObjectHitDelta(ObjectState* obj);

	// Processed button press events, returns the hit object, if any
	ObjectState* OnButtonPressed(uint32 buttonCode);
	// Processed button release events
	void OnButtonReleased(uint32 buttonCode);

	// Check if a hold object or laser is currently being held
	bool IsActive(ObjectState* object) const;
	bool IsLaserActive() const;

	// Get laser roll values
	// these range from -1 to 1, 0 being the center, and 1 the outer extreme
	float GetLaserRollOutput(uint32 index);
	// This combines the 2 active lasers to always generate a 0-1 value
	// This is the value that goes towards the filter on the music controlled by the lasers
	float GetLaserOutput();

	// Calculates the maximum score of the current map
	uint32 CalculateMaxScore() const;

	Delegate<uint32, MapTime> OnButtonScore;
	Delegate<uint32, ObjectState*> OnButtonHit;
	Delegate<uint32> OnButtonMiss;
	// Idx, dir, newPos
	Delegate<uint32, float, float> OnLaserSlamHit;
	Delegate<uint32> OnComboChanged;

	// The maximum timing window in which you are able to hit an object early
	static const MapTime maxEarlyHitTime;
	static const MapTime perfectHitTime;
	// Maximum timing window in which you can hit laser segments
	static const MapTime maxLaserHitTime;

	// Movement speed of the lasers when idle
	static const float idleLaserMoveSpeed;

	// Maximum accumulated score of object that have been hit or missed
	uint32 currentMaxScore;
	// The total maximun score of a map were all objects to be hit perfectly
	uint32 totalMaxScore;
	// The actual amount of gotten score
	uint32 currentHitScore;

	// Current combo
	uint32 currentComboCounter;

	// The timings of hit objects, sorted by time hit
	// this is mainly for debugging purposes
	Vector<HitStat> hitStats;

	// Autoplay mode
	bool autoplay = false;

	// Laser Objects
	float laserInput[2];
	float laserPositions[2];
	float laserTargetPositions[2] = { 0 };
	// Always points to the currently active laser segment
	LaserObjectState* activeLaserObjects[2] = { 0 };

	// Used for hold logic on laser segments (non-instant segments)
	LaserObjectState* laserHoldObjects[2] = { 0 };
	double laserHoldDuration[2] = { 0 };
	double laserMissDuration[2] = { 0 };
	bool laserActive[2] = { false };

	// Hold Objects
	HoldObjectState* activeHoldObjects[6] = { 0 };
	double lastHoldDuration[6] = { 0 };

private:
	// Sets the new interpolation values for laser output
	void m_UpdateLaserOutput(float deltaTime, bool interpolate = true);
	// Laser output without interpolation
	float m_GetLaserOutputRaw();
	void m_RegisterHit(ObjectState* obj);
	void m_OnObjectEntered(ObjectState* obj);
	void m_OnObjectLeaved(ObjectState* obj);
	void m_AddCombo(uint32 amount = 1);
	void m_ResetCombo();
	float m_SampleLaserPosition(MapTime time, LaserObjectState* laser);

	// For hold note ticks
	uint32 m_holdTickCounter;
	MapTime m_lastTime;

	// Lerp for laser output
	float m_laserOutputSource = 0.0f;
	float m_laserOutputTarget = 0.0f;
	float m_timeSinceOutputSet = 0.0f;

	class BeatmapPlayback* m_playback;
};

