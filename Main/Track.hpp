#pragma once
#include "Scoring.hpp"
#include "AsyncLoadable.hpp"

/*
	The object responsible for drawing the track.
*/
class Track : Unique, public IAsyncLoadable
{
public:
	// Size constants of various elements
	static const float trackWidth;
	static const float trackLength;
	static const float buttonWidth;
	static const float laserWidth;
	static const float fxbuttonWidth;
	static const float buttonTrackWidth;

	float trackTickLength;
	float buttonLength;
	float fxbuttonLength;

	// Laser color setting
	Color laserColors[2] = {};

	// Hit effect color settings
	// 0 = Miss
	// 1 = Good
	// 2 = Perfect
	// 3 = Idle
	Color hitColors[4] = {};

	class AsyncAssetLoader* loader = nullptr;

public:
	Track();
	~Track();
	virtual bool AsyncLoad() override;
	virtual bool AsyncFinalize() override;
	void Tick(class BeatmapPlayback& playback, float deltaTime);

	// Just the board with tick lines
	void DrawBase(RenderQueue& rq);
	// Draws an object
	void DrawObjectState(RenderQueue& rq, class BeatmapPlayback& playback, ObjectState* obj, bool active = false);
	// Things like the laser pointers, hit bar and effect
	void DrawOverlays(RenderQueue& rq);
	// Draws a plane over the track
	void DrawTrackOverlay(RenderQueue& rq, Texture texture, float heightOffset = 0.05f, float widthScale = 1.0f);
	// Draw a centered sprite at pos, relative from the track
	void DrawSprite(RenderQueue& rq, Vector3 pos, Vector2 size, Texture tex, Color color = Color::White, float tilt = 0.0f);
	void DrawCombo(RenderQueue& rq, uint32 score, Color color, float scale = 1.0f);

	// Adds a sprite effect to the track
	struct TimedEffect* AddEffect(struct TimedEffect* effect);
	void ClearEffects();

	void SetViewRange(float newRange);
	float GetViewRange() const;

	// Normal/FX button X-axis placement
	float GetButtonPlacement(uint32 buttonIdx);

	// Laser positions, as shown on the overlay
	float laserPositions[2];
	float laserPointerOpacity[2] = { 1.0f };

	// Visible time elements on the playfield track
	// a single unit is 1 beat in distance
	Vector2 trackViewRange;

	/* Track base graphics */
	Mesh trackMesh;
	Mesh trackTickMesh;
	Material trackMaterial; // Also used for buttons
	Texture trackTexture;
	Texture trackTickTexture;

	/* Object graphics */
	Mesh buttonMesh;
	Texture buttonTexture;
	Texture buttonHoldTexture;
	Mesh fxbuttonMesh;
	Texture fxbuttonTexture;
	Texture fxbuttonHoldTexture;
	Material holdButtonMaterial;
	Texture laserTexture;
	Texture laserTailTextures[2]; // Entry and exit textures
	Material laserMaterial;

	/* Overlay graphics */
	Material trackOverlay;

	/* Scoring and feedback elements */
	Texture scoreBarTexture;
	Texture scoreHitTexture;
	Texture laserPointerTexture;
	Texture scoreHitTextures[3]; // Ok, Miss, Perfect
	// Combo counter sprite sheet
	Texture comboSpriteSheet;
	Mesh comboSpriteMeshes[10];
	/* Reusable sprite mesh and material */
	Mesh centeredTrackMesh;
	Material spriteMaterial;

	// For flicking objects, like hold objects that are active
	float objectGlow;

private:
	// Laser track generators
	class LaserTrackBuilder* m_laserTrackBuilder[2] = { 0 };

	const TimingPoint* m_lastTimingPoint;

	// Bar tick locations
	Vector<float> m_barTicks;

	// Active effects
	Vector<struct TimedEffect*> m_hitEffects;

	// Distance of seen objects on the track
	float m_viewRange;

	MapTime m_lastMapTime = 0;
}; 

// Base class for sprite effects on the track
struct TimedEffect
{
	TimedEffect(float duration);;
	virtual ~TimedEffect() = default;
	void Reset(float duration);
	float GetRate() const { return time / duration; }
	virtual void Draw(class RenderQueue& rq) = 0;
	virtual void Tick(float deltaTime);

	Track* track;
	float duration;
	float time;
};

// Button hit effect
struct ButtonHitEffect : public TimedEffect
{
	ButtonHitEffect(uint32 buttonCode, Color color);;
	virtual void Draw(class RenderQueue& rq) override;

	uint32 buttonCode;
	Color color;
};
// Button hit rating effect
struct ButtonHitRatingEffect : public TimedEffect
{
	ButtonHitRatingEffect(uint32 buttonCode, ScoreHitRating rating);;
	virtual void Draw(class RenderQueue& rq) override;

	uint32 buttonCode;
	ScoreHitRating rating;
};