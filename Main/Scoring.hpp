#pragma 
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
	bool operator<(const HitStat& other) { return time < other.time; }
	uint64 time;
	int64 delta;
};

/*
	Calculates game score and checks which objects are hit
	also keeps track of laser positions
*/
class Scoring
{
public:
	Scoring();

	// The maximum timeframe in which you are able to hit an object early
	static const int64 maxEarlyHitTime;
	static const int64 perfectHitTime;

	// Updates the list of objects that are possible to hit
	void Tick(Vector<ObjectState*> objects, MapTime currentTime, float deltaTime);

	// Handle button presses (bt 0-3, fx 4-5)
	void HandleButtonPress(uint32 buttonCode);
	void HandleButtonRelease(uint32 buttonCode);

	ScoreHitRating GetScoreHitRatingFromMs(int64 delta);

	// Retrieves the delt for a button that was previously hit
	int64 GetObjectHitDelta(ObjectState* state, uint32 buttonCode);

	int64 GetAverageHitDelta() const;

	Delegate<ObjectState*, uint32> OnButtonHit;

	// Maximum accumulated score of object that have been hit or missed
	uint32 currentMaxScore;
	// The actual amount of gotten score
	uint32 currentHitScore;

	// Current combo
	uint32 currentComboCounter;

	// Hold objects for every button and laser
	ObjectState* currentHoldObjects[8] = { 0 };
	MapTime holdDurations[8] = { 0 };
	int64 holdStartDelta[8] = { 0 };

	// Laser positions
	float laserPositions[2];
	float laserInput[2] = { 0.0f };
	static const float idleLaserMoveSpeed;

	Map<ObjectState*, ObjectHitState> recentHits;

	// Debug array, contains times of hit notes and how much the timing was off
	Vector<HitStat> hitStats;

private:
	// Checks if an object was already hit, returns true if it was not and makes sure this does not trigger again
	bool m_RecordHit(ObjectState* state, uint32 buttonCode);
	void m_AddScore(ObjectState* state, uint32 buttonCode, int64 delta);
	void m_TerminateHoldObject(uint32 buttonCode);
	bool buttonStates[6];

	// USed to calculate average hit delay
	int64 m_hitNotesDelta;
	int64 m_numNotesHit;

	Vector<ObjectState*> m_hitableObjects;
	MapTime m_currentTime;
};

