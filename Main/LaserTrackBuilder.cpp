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
Mesh LaserTrackBuilder::GenerateTrackMesh(class BeatmapPlayback& playback, LaserObjectState* laser)
{
	if(m_objectCache.Contains(laser))
		return m_objectCache[laser];

	Mesh newMesh = MeshRes::Create(m_gl);

	// Calculate amount to scale laser size to fit the texture border in
	assert(laserTextureSize.x == laserTextureSize.y); // Use Square texture
	float laserTextureScale = ((float)laserTextureSize.x / (float)(laserTextureSize.x - laserBorderPixels));
	float laserBorderAmount = laserTextureScale - 1.0f;

	float laserWidthWithBorder = m_laserWidth * laserTextureScale;
	float trackWidthWidthBorder = m_trackWidth + laserBorderAmount * m_laserWidth;
	float effectiveWidth = trackWidthWidthBorder - laserWidthWithBorder;
	float halfWidth = laserWidthWithBorder * 0.5f;
	float length = playback.DurationToBarDistance(laser->duration);
	float textureBorder = 0.25f;
	float invTextureBorder = 1.0f - textureBorder;

	if((laser->flags & LaserObjectState::flag_Instant) != 0) // Slam segment
	{
		float left, right;
		float top = length;
		float bottom = 0.0f;
		float halfBorder = length * laserBorderAmount * 0.5f;
		bottom -= halfBorder;
		top += halfBorder;
		
		left = laser->points[0] * effectiveWidth - effectiveWidth * 0.5f;
		right = laser->points[1] * effectiveWidth - effectiveWidth * 0.5f;

		// If corners should be placed, connecting the texture to the previous laser
		bool cornerRight = laser->next != nullptr;
		bool cornerLeft = laser->prev != nullptr;
		bool swapped = false;
		if(laser->points[0] > laser->points[1])
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
			{ { left,  top,     0.0f },{ textureBorder, textureBorder } },
			{ { right, bottom,  0.0f },{ invTextureBorder, invTextureBorder } },
			{ { right, top,     0.0f },{ invTextureBorder, textureBorder } },

			{ { left,  top,     0.0f },{ textureBorder, textureBorder } },
			{ { left,  bottom,  0.0f },{ textureBorder, invTextureBorder } },
			{ { right, bottom,  0.0f },{ invTextureBorder, invTextureBorder } },
		};

		// Generate corners
		if(cornerLeft)
		{
			float cleft = left - laserWidthWithBorder;
			float cright = left;

			Vector<MeshGenerators::SimpleVertex> cl =
			{
				{ { cleft,  top,     0.0f },{ textureBorder, textureBorder } },
				{ { cright, bottom,  0.0f },{ invTextureBorder, invTextureBorder } },
				{ { cright, top,     0.0f },{ textureBorder, textureBorder } },

				{ { cleft,  top,     0.0f },{ textureBorder, textureBorder } },
				{ { cleft,  bottom,  0.0f },{ textureBorder, invTextureBorder } },
				{ { cright, bottom,  0.0f },{ invTextureBorder, invTextureBorder } },
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
				{ { cleft,  top,     0.0f },{ textureBorder, textureBorder } },
				{ { cright, bottom,  0.0f },{ invTextureBorder, invTextureBorder } },
				{ { cright, top,     0.0f },{ invTextureBorder, textureBorder } },

				{ { cleft,  top,     0.0f },{ textureBorder, textureBorder } },
				{ { cleft,  bottom,  0.0f },{ textureBorder, invTextureBorder } },
				{ { cright, bottom,  0.0f },{ invTextureBorder,invTextureBorder } },
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
			Vector2(laser->points[0] * effectiveWidth - effectiveWidth * 0.5f, 0.0f), // Bottom
			Vector2(laser->points[1] * effectiveWidth - effectiveWidth * 0.5f, length), // Top
		};

		Vector<MeshGenerators::SimpleVertex> verts =
		{
			{ { points[0].x - halfWidth, points[0].y,  0.0f },{ 0.0f, invTextureBorder} }, // BL
			{ { points[0].x + halfWidth, points[0].y,  0.0f },{ 1.0f, invTextureBorder} }, // BR
			{ { points[1].x + halfWidth, points[1].y,  0.0f },{ 1.0f, textureBorder } }, // TR

			{ { points[0].x - halfWidth, points[0].y,  0.0f },{ 0.0f, invTextureBorder } }, // BL
			{ { points[1].x + halfWidth, points[1].y,  0.0f },{ 1.0f, textureBorder } }, // TR
			{ { points[1].x - halfWidth, points[1].y,  0.0f },{ 0.0f, textureBorder } }, // TL
		};

		newMesh->SetData(verts);
		newMesh->SetPrimitiveType(PrimitiveType::TriangleList);
	}

	// Cache this mesh
	m_objectCache.Add(laser, newMesh);
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
		LaserObjectState* obj = it->first;
		MapTime endTime = obj->time + obj->duration + 1000;
		if(newTime > endTime)
		{
			it = m_objectCache.erase(it);
			continue;
		}
		it++;
	}
}