#pragma once
#include "BeatmapObjects.hpp"

// Hit rating for hitting a button
enum class ScoreHitRating
{
	Miss = 0,
	Good,
	Perfect,
	Idle, // Not actual score, used when a button is pressed when there are no notes
};


// Hit statistic
struct HitStat
{
	HitStat(ObjectState* object);
	bool operator<(const HitStat& other);

	ObjectState* object;

	MapTime time;
	MapTime delta = 0;
	ScoreHitRating rating = ScoreHitRating::Miss;

	// Hold state
	// This is the amount of gotten ticks in a hold sequence
	uint32 hold = 0;
	// This is the amount of total ticks in this hold sequence
	uint32 holdMax = 0;
	// If at least one hold tick has been missed
	bool hasMissed = false;
};