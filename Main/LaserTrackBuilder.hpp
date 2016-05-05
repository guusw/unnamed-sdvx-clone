#pragma once
#include "BeatmapObjects.hpp"
#include "Mesh.hpp"
#include "Texture.hpp"

class LaserTrackBuilder
{
public:
	LaserTrackBuilder(class OpenGL* gl, uint32 laserIndex, float trackWidth, float laserWidth);
	void Reset();
	void Update(MapTime newTime);

	// Generates a normal segment
	Mesh GenerateTrackMesh(class BeatmapPlayback& playback, LaserObjectState* laser);

	// Generate the starting segment of a laser
	Mesh GenerateTrackEntry(class BeatmapPlayback& playback, LaserObjectState* laser);
	// Generate the ending segment of a laser
	Mesh GenerateTrackExit(class BeatmapPlayback& playback, LaserObjectState* laser);

	// Used to generate larges meshes but allow the texture to match the actual laser width
	uint32 laserBorderPixels;
	Vector2i laserTextureSize;

	// Size of both the entry and exit texture
	//	the width should be the same size as the normal laser texture
	//	it should also have the same border placement
	Vector2i laserEntryTextureSize;
	Vector2i laserExitTextureSize;

	// The length scale based on the view distance
	float laserLengthScale;
	float perspectiveHeightScale;

private:
	// The uv coordinates to sample the laser without the border, or only the border
	float textureBorder;
	float invTextureBorder;

	// The the size of the laser with compensation added for the border
	float actualLaserWidth;

	// The width of the laser without the border
	float laserWidthNoBorder;
	// World size of a single border around a laser
	float realBorderSize;

	// The length of the horizontal slam segments
	float slamLength;

	// The effective area in which the center point of the laser can move
	float effectiveWidth;

private:
	void m_RecalculateConstants();
	void m_Cleanup(MapTime newTime, Map<LaserObjectState*, Mesh>& arr);
	class OpenGL* m_gl;

	float m_trackWidth;
	float m_laserWidth;
	uint32 m_laserIndex;
	Map<LaserObjectState*, Mesh> m_objectCache;
	Map<LaserObjectState*, Mesh> m_cachedEntries;
	Map<LaserObjectState*, Mesh> m_cachedExits;
};