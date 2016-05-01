#include "stdafx.h"
#include "LaserTrackBuilder.hpp"
#include "MeshGenerators.hpp"
#include "BeatmapPlayback.hpp"
#include <algorithm>

LaserTrackBuilder::LaserTrackBuilder(class OpenGL* gl, uint32 laserIndex, float trackWidth, float laserWidth)
{
	m_gl = gl;
	m_laserIndex = laserIndex;
	m_trackWidth = trackWidth;
	m_laserWidth = laserWidth;
}
Mesh LaserTrackBuilder::GenerateTrackMesh(class BeatmapPlayback& playback, ObjectState* state)
{
	if(m_objectCache.Contains(state))
		return m_objectCache[state];

	const LaserState& laser = state->lasers[m_laserIndex];
	Mesh newMesh = MeshRes::Create(m_gl);

	// Get following 
	const LaserState* nextLaser = nullptr;
	const LaserState* prevLaser = nullptr;
	if(laser.next)
	{
		nextLaser = &laser.next->lasers[m_laserIndex];
	}
	if(laser.prev)
	{
		prevLaser = &laser.prev->lasers[m_laserIndex];
	}


	// Calculate amount to scale laser size to fit the texture border in
	assert(laserTextureSize.x == laserTextureSize.y); // Use Square texture
	float laserTextureScale = ((float)laserTextureSize.x / (float)(laserTextureSize.x - laserBorderPixels));
	float laserBorderAmount = laserTextureScale - 1.0f;

	float laserWidthWithBorder = m_laserWidth * laserTextureScale;
	float trackWidthWidthBorder = m_trackWidth + laserBorderAmount * m_laserWidth;
	float effectiveWidth = trackWidthWidthBorder - laserWidthWithBorder;
	float halfWidth = laserWidthWithBorder * 0.5f;
	float length = playback.DurationToBarDistance(laser.duration);

	if(playback.IsLaserSlam(laser, state->time)) // Slam segment
	{
		float left, right;
		float top = length;
		float bottom = 0.0f;
		float halfBorder = length * laserBorderAmount * 0.5f;
		bottom -= halfBorder;
		top += halfBorder;
		
		left = laser.points[0] * effectiveWidth - effectiveWidth * 0.5f;
		right = laser.points[1] * effectiveWidth - effectiveWidth * 0.5f;

		// If corners should be placed, connecting the texture to the previous laser
		bool cornerRight = nextLaser != nullptr;
		bool cornerLeft = prevLaser != nullptr;
		bool swapped = false;
		if(laser.points[0] > laser.points[1])
		{
			// <------
			std::swap(left, right);
			std::swap(cornerLeft, cornerRight);
			swapped = true;
		}// else ------>

		// Make place for corner pieces
		if(cornerLeft)
			left += m_laserWidth * 0.5f;
		else // otherwise extend all the way from the center to the edge
			left -= laserWidthWithBorder * 0.5f;
		if(cornerRight)
			right -= m_laserWidth * 0.5f;
		else
			right += laserWidthWithBorder * 0.5f;

		Vector<MeshGenerators::SimpleVertex> verts =
		{
			{ { left,  top,     0.0f },{ 0.25f, 0.0f } },
			{ { right, bottom,  0.0f },{ 0.75f, 1.0f } },
			{ { right, top,     0.0f },{ 0.75f, 0.0f } },

			{ { left,  top,     0.0f },{ 0.25f, 0.0f } },
			{ { left,  bottom,  0.0f },{ 0.25f, 1.0f } },
			{ { right, bottom,  0.0f },{ 0.75f, 1.0f } },
		};

		// Generate corners
		if(cornerLeft)
		{
			float cleft = left - laserWidthWithBorder;
			float cright = left;

			Vector<MeshGenerators::SimpleVertex> cl =
			{
				{ { cleft,  top,     0.0f },{ 0.0f, 0.0f } },
				{ { cright, bottom,  0.0f },{ 0.75f, 1.0f } },
				{ { cright, top,     0.0f },{ 0.0f, 0.0f } },

				{ { cleft,  top,     0.0f },{ 0.0f, 0.0f } },
				{ { cleft,  bottom,  0.0f },{ 0.0f, 1.0f } },
				{ { cright, bottom,  0.0f },{ 0.75f, 1.0f } },
			};
			for(auto& v : cl)
				verts.Add(v);
		}
		if(cornerRight)
		{
			float cleft = right;
			float cright = right + laserWidthWithBorder;

			Vector<MeshGenerators::SimpleVertex> cl =
			{
				{ { cleft,  top,     0.0f },{ 0.25f, 0.0f } },
				{ { cright, bottom,  0.0f },{ 1.0f, 1.0f } },
				{ { cright, top,     0.0f },{ 1.0f, 0.0f } },

				{ { cleft,  top,     0.0f },{ 0.25f, 0.0f } },
				{ { cleft,  bottom,  0.0f },{ 0.25f, 1.0f } },
				{ { cright, bottom,  0.0f },{ 1.0f, 1.0f } },
			};
			for(auto& v : cl)
				verts.Add(v);
		}

		newMesh->SetData(verts);
		newMesh->SetPrimitiveType(PrimitiveType::TriangleList);
	}
	else
	{
		Vector2 points[2] =
		{
			Vector2(laser.points[0] * effectiveWidth - effectiveWidth * 0.5f, 0.0f), // Bottom
			Vector2(laser.points[1] * effectiveWidth - effectiveWidth * 0.5f, length), // Top
		};

		Vector<MeshGenerators::SimpleVertex> verts =
		{
			{ { points[0].x - halfWidth, points[0].y,  0.0f },{ 0.0f, 0.75f } }, // BL
			{ { points[0].x + halfWidth, points[0].y,  0.0f },{ 1.0f, 0.75f } }, // BR
			{ { points[1].x + halfWidth, points[1].y,  0.0f },{ 1.0f, 0.25f } }, // TR

			{ { points[0].x - halfWidth, points[0].y,  0.0f },{ 0.0f, 0.25f } }, // BL
			{ { points[1].x + halfWidth, points[1].y,  0.0f },{ 1.0f, 0.25f } }, // TR
			{ { points[1].x - halfWidth, points[1].y,  0.0f },{ 0.0f, 0.25f } }, // TL
		};

		newMesh->SetData(verts);
		newMesh->SetPrimitiveType(PrimitiveType::TriangleList);
	}

	// Cache this mesh
	m_objectCache.Add(state, newMesh);

	return newMesh;
}
void LaserTrackBuilder::Reset()
{
	m_objectCache.clear();
}

void LaserTrackBuilder::Update(MapTime newTime)
{
	// Cleanup unused meshes
	for(auto it = m_objectCache.begin(); it != m_objectCache.end();)
	{
		ObjectState* obj = it->first;
		MapTime endTime = obj->time + obj->maxDuration + 1000;
		if(newTime > endTime)
		{
			it = m_objectCache.erase(it);
			continue;
		}
		it++;
	}
}