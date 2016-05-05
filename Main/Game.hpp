#pragma once
#include "ApplicationTickable.hpp"

/*
	Main game scene / logic manager
*/
class Game : public IApplicationTickable
{
protected:
	Game() = default;
public:
	virtual ~Game() = default;
	static Game* Create(class Beatmap* map, String mapPath);

public:
	// Starts a new map 
	virtual bool Init(Beatmap* map, String mapPath) = 0;
	// When the game is still going, false when the map is done, all ending sequences have played, etc.
	// also false when the player leaves the game
	virtual bool IsPlaying() const = 0;

	virtual class Track& GetTrack() = 0;
	virtual class Camera& GetCamera() = 0;
	virtual class BeatmapPlayback& GetPlayback() = 0;
	virtual class Scoring& GetScoring() = 0;
};