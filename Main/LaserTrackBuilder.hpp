#pragma once
#include "Beatmap.hpp"
#include "Mesh.hpp"
#include "Texture.hpp"

class LaserTrackBuilder
{
public:
	LaserTrackBuilder(class OpenGL* gl, uint32 laserIndex, float trackWidth, float laserWidth);
	void Reset();
	void Update(MapTime newTime);
	Mesh GenerateTrackMesh(class BeatmapPlayback& playback, ObjectState* state);
	
	TimingPoint currentTimingPoint;

	// Used to generate larges meshes but allow the texture to match the actual laser width
	uint32 laserBorderPixels;
	Vector2i laserTextureSize;

private:
	class OpenGL* m_gl;

	float m_trackWidth;
	float m_laserWidth;
	uint32 m_laserIndex;
	Map<ObjectState*, Mesh> m_objectCache;
};