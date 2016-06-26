#pragma once
#include "BeatmapPlayback.hpp"
#include "HitStat.hpp"
#include "Input.hpp"

enum class TickFlags : uint8
{
	None = 0,
	// Used for segment start/end parts
	Start = 0x1,
	End = 0x2,
	// Hold notes (BT or FX)
	Hold = 0x4,
	// Normal/Single hit buttons
	Button = 0x8,
	// For lasers only
	Laser = 0x10,
	Slam = 0x20, 
};
TickFlags operator|(const TickFlags& a, const TickFlags& b);
TickFlags operator&(const TickFlags& a, const TickFlags& b);

// Tick object to record hits
struct ScoreTick
{
public:
	ScoreTick() = default;
	ScoreTick(ObjectState* object) : object(object) {};

	// Returns the time frame in which this tick can be hit
	MapTime GetHitWindow() const;
	// Hit rating when hitting object at given time
	ScoreHitRating GetHitRating(MapTime currentTime) const;
	// Check a flag
	bool HasFlag(TickFlags flag) const;
	void SetFlag(TickFlags flag);

	TickFlags flags = TickFlags::None;
	MapTime time;
	ObjectState* object = nullptr;
};

/*
	Calculates game score and checks which objects are hit
	also keeps track of laser positions
*/
class Scoring : public Unique
{
public:
	Scoring();
	~Scoring();

	// Needs to be set to find out which objects are active/hittable
	void SetPlayback(BeatmapPlayback& playback);

	// Needs to be set to handle input
	void SetInput(Input* input);

	// Resets/Initializes the scoring system
	// Called after SetPlayback
	void Reset();

	// Updates the list of objects that are possible to hit
	void Tick(float deltaTime);

	float GetLaserRollOutput(uint32 index);
	float GetLaserOutput();

	// Check if an object is currently held
	//	works only for lasers and hold buttons
	bool IsObjectHeld(ObjectState* object);
	// Check if an object is currently held, by object index
	//	Buttons[0,5], Lasers[6,7]
	bool IsObjectHeld(uint32 index) const;
	// Check if a laser is currently held
	bool IsLaserHeld(uint32 laserIndex) const;

	// Checks if a laser is currently not used or needed soon
	bool IsLaserIdle(uint32 index) const;

	// Calculates the maximum score of the current map
	// The score is calculated per 2 (2 = critical, 1 = near)
	// Hold buttons, lasers, etc. give 2 points per tick
	uint32 CalculateMaxScore() const;

	// Actual score, in the range 0-10,000,000
	uint32 CalculateCurrentScore() const;

	// Called when a hit is recorded on a given button index (excluding hold notes)
	// (Hit Button, Score, Hit Object(optional))
	Delegate<Input::Button, ScoreHitRating, ObjectState*> OnButtonHit;
	// Called when a miss is recorded on a given button index
	Delegate<Input::Button> OnButtonMiss;

	// Called when an object is picked up
	Delegate<Input::Button, ObjectState*> OnObjectHold;
	// Called when an object is let go of
	Delegate<Input::Button, ObjectState*> OnObjectReleased;

	// Called when a laser slam was hit
	// (Laser slam segment)
	Delegate<LaserObjectState*> OnLaserSlamHit;
	// Called when the combo counter changed
	// (New Combo)
	Delegate<uint32> OnComboChanged;

	// Object timing window
	static const MapTime goodHitTime;
	static const MapTime perfectHitTime;
	static const float idleLaserSpeed;

	// Maximum accumulated score of object that have been hit or missed
	// used to calculate accuracy up to a give point
	uint32 currentMaxScore = 0;
	// The total maximum score of a map were all objects to be hit perfectly
	uint32 totalMaxScore = 0;
	// The actual amount of gotten score
	uint32 currentHitScore = 0;

	// Current combo
	uint32 currentComboCounter;

	// The timings of hit objects, sorted by time hit
	// these are used for debugging
	Vector<HitStat*> hitStats;

	// Autoplay mode
	bool autoplay = false;

	// Actual positions of the laser
	float laserPositions[2];
	// Sampled target position of the lasers in the map
	float laserTargetPositions[2] = { 0 };

private:
	// Calculates the times at which a single hold object ticks
	void m_CalculateHoldTicks(HoldObjectState* hold, Vector<MapTime>& ticks) const;
	// Calculates the times at which a single laser chain object ticks
	//	use the root laser object
	void m_CalculateLaserTicks(LaserObjectState* laserRoot, Vector<ScoreTick>& ticks) const;
	void m_OnObjectEntered(ObjectState* obj);
	void m_OnObjectLeaved(ObjectState* obj);

	// Button event handlers
	void m_OnButtonPressed(Input::Button buttonCode);
	void m_OnButtonReleased(Input::Button buttonCode);
	void m_CleanupInput();

	// Updates all pending ticks
	void m_UpdateTicks();
	// Tries to trigger a hit event on an approaching tick
	ObjectState* m_ConsumeTick(uint32 buttonCode);
	// Called whenether missed or not
	void m_OnTickProcessed(ScoreTick* tick, uint32 index);
	void m_TickHit(ScoreTick* tick, uint32 index, MapTime delta = 0);
	void m_TickMiss(ScoreTick* tick, uint32 index);

	// Called when score is gained
	//	should only be called once for a single object since this also increments the combo counter
	void m_AddScore(uint32 score);
	// Reset combo counter
	void m_ResetCombo();

	// Sets a held object
	void m_SetHoldObject(ObjectState* obj, uint32 index);
	void m_ReleaseHoldObject(ObjectState* obj);
	void m_ReleaseHoldObject(uint32 index);

	// Updates the target laser positions and currently tracked laser segments for those
	//  also updates laser input and returns lasers back to idle position when not used
	void m_UpdateLasers(float deltaTime);

	// Returns the raw laser output value, not interpolated
	float m_GetLaserOutputRaw();
	void m_UpdateLaserOutput(float deltaTime);

	// Creates or retrieves an existing hit stat and returns it
	HitStat* m_AddOrUpdateHitStat(ObjectState* object);

	// Updates laser output with or without interpolation
	bool m_interpolateLaserOutput = false;

	// Lerp for laser output
	float m_laserOutputSource = 0.0f;
	float m_laserOutputTarget = 0.0f;
	float m_timeSinceOutputSet = 0.0f;

	class Input* m_input = nullptr;
	class BeatmapPlayback* m_playback = nullptr;

	// Input values for laser [-1,1]
	float m_laserInput[2] = { 0.0f };
	float m_timeSinceLaserInput[2] = { 0.0f };

	// used the update the amount of hit ticks for hold/laser notes
	Map<ObjectState*, HitStat*> m_holdHitStats;

	// Laser objects currently in range
	//	used to sample target laser positions
	LaserObjectState* m_currentLaserSegments[2] = { nullptr };
	// Queue for the above list
	Vector<LaserObjectState*> m_laserSegmentQueue;

	// Ticks for each BT[4] / FX[2] / Laser[2]
	Vector<ScoreTick*> m_ticks[8];
	// Hold objects
	ObjectState* m_holdObjects[8];
	Set<ObjectState*> m_heldObjects;
};

