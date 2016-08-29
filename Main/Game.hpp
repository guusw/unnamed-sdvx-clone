#pragma once
#include "ApplicationTickable.hpp"
#include "AsyncLoadable.hpp"

/*
	Main game scene / logic manager
*/
class Game : public IAsyncLoadableApplicationTickable
{
protected:
	Game() = default;
public:
	virtual ~Game() = default;
	static Game* Create(const String& mapPath);

public:
	// When the game is still going, false when the map is done, all ending sequences have played, etc.
	// also false when the player leaves the game
	virtual bool IsPlaying() const = 0;

	virtual class Track& GetTrack() = 0;
	virtual class Camera& GetCamera() = 0;
	virtual class BeatmapPlayback& GetPlayback() = 0;
	virtual class Scoring& GetScoring() = 0;
	// Map jacket image
	virtual Texture GetJacketImage() = 0;
	// The beatmap
	virtual Ref<class Beatmap> GetBeatmap() = 0;
};