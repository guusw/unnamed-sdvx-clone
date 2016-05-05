#pragma once
#include "Scoring.hpp"
#include "Mesh.hpp"
#include "Material.hpp"
#include "Texture.hpp"

/*
	The object responsible for drawing the track.
*/
class Track : Unique
{
public:
	// Size constants of various elements
	const float trackWidth = 1.0f;
	const float trackLength = 25.0f;
	const float buttonWidth = trackWidth / 6;
	const float laserWidth = buttonWidth * 0.8f;
	const float fxbuttonWidth = buttonWidth * 2;
	const float buttonTrackWidth = buttonWidth * 4;

	/// TODO: Use BPM scale for view range
	const float viewRange = 0.5f;

	float trackTickLength;
	float buttonLength;
	float fxbuttonLength;

	// This is due to the viewing angle of the camera, objects should be longer to appear their normal size
	// This gets applied to all objects' Y direction drawn parallel to the track.
	float perspectiveHeightScale = 5.0f;

	// Laser color setting
	Color laserColors[2] = {};

	// Hit effect color settings
	// 0 = Idle
	// 1 = Perfect
	// 2 = Good
	// 3 = Miss
	Color hitColors[4] = {};

public:
	~Track();
	bool Init();
	void Tick(class BeatmapPlayback& playback, float deltaTime);

	// Just the board with tick lines
	void DrawBase(class RenderQueue& rq);
	// Draws an object
	void DrawObjectState(class RenderQueue& rq, class BeatmapPlayback& playback, ObjectState* obj, bool active = false);
	// Things like the laser pointers, hit bar and effect
	void DrawOverlays(class RenderQueue& rq);

	// Draws a plane over the track
	void DrawTrackOverlay(class RenderQueue& rq, Texture texture, float heightOffset = 0.05f, float widthScale = 1.0f);
	// Draw a centered sprite at pos, relative from the track
	void DrawSprite(RenderQueue& rq, Vector3 pos, Vector2 size, Texture tex, Color color = Color::White, float tilt = 0.0f);

	// Adds a sprite effect to the track
	struct TimedEffect* AddEffect(struct TimedEffect* effect);

	// Laser positions, as shown on the overlay
	float laserPositions[2];

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
	/* Reusable sprite mesh and material */
	Mesh centeredTrackMesh;
	Material spriteMaterial;

	// For flicking objects, like hold objects that are active
	float objectGlow;

private:
	// Laser track generators
	class LaserTrackBuilder* m_laserTrackBuilder[2] = { 0 };

	// Active effects
	Vector<struct TimedEffect*> m_hitEffects;

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