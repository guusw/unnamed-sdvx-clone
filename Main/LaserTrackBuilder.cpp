#include "stdafx.h"
#include "LaserTrackBuilder.hpp"
#include "BeatmapPlayback.hpp"
#include "Track.hpp"
#include <algorithm>

LaserTrackBuilder::LaserTrackBuilder(class OpenGL* gl, class Track* track, uint32 laserIndex)
{
	m_gl = gl;
	m_track = track;
	m_laserIndex = laserIndex;
	m_trackWidth = track->trackWidth;
	m_laserWidth = track->laserWidth;
	laserTextureSize = track->laserTexture->GetSize();
	laserEntryTextureSize = track->laserTailTextures[0]->GetSize();
	laserExitTextureSize = track->laserTailTextures[1]->GetSize();
}
Mesh LaserTrackBuilder::GenerateTrackMesh(class BeatmapPlayback& playback, LaserObjectState* laser)
{
	if(m_objectCache.Contains(laser))
		return m_objectCache[laser];

	Mesh newMesh = MeshRes::Create(m_gl);

	float length = playback.DurationToViewDistanceAtTime(laser->time, laser->duration);

	if((laser->flags & LaserObjectState::flag_Instant) != 0) // Slam segment
	{
		float left, right;
		left = laser->points[0] * effectiveWidth - effectiveWidth * 0.5f;
		right = laser->points[1] * effectiveWidth - effectiveWidth * 0.5f;


		// If corners should be placed, connecting the texture to the previous laser
		bool swapped = false;
		if(laser->points[0] > laser->points[1])
		{
			// <------
			std::swap(left, right);
			swapped = true;
		}// else ------>

		// Make place for corners
		left += laserWidthNoBorder * 0.5f;
		right -= laserWidthNoBorder * 0.5f;

		// Uv coordinates for center and borders
		// More tight border in horizontal direction
		Rect centerMiddleUv = Rect(textureBorder, textureBorder, invTextureBorder, invTextureBorder);
		Rect centerUpperUv = Rect(textureBorder, 0.0f, invTextureBorder, textureBorder);
		Rect centerLowerUv = Rect(textureBorder, invTextureBorder, invTextureBorder, 1.0f);

		// Generate positions for middle top and bottom
		float slamLength = playback.DurationToViewDistanceAtTime(laser->time, slamDuration) * laserLengthScale;
		Rect3D centerMiddle = Rect3D(left, slamLength, right, 0.0f);
		Rect3D centerBottom = centerMiddle;
		centerBottom.size.y = realBorderSize;
		centerBottom.pos.y = centerMiddle.Bottom() - centerBottom.size.y;
		Rect3D centerTop = centerBottom;
		centerTop.pos.y = centerMiddle.Top();

		float uvMin = textureBorder;
		float uvMax = invTextureBorder;

		Vector<MeshGenerators::SimpleVertex> verts;

		// Middle part
		MeshGenerators::GenerateSimpleXYQuad(centerTop, centerUpperUv, verts);
		MeshGenerators::GenerateSimpleXYQuad(centerBottom, centerLowerUv, verts);
		MeshGenerators::GenerateSimpleXYQuad(centerMiddle, centerMiddleUv, verts);

		// Generate left corner
		{
			Rect3D leftCenter = Rect3D(left - laserWidthNoBorder, centerMiddle.Top(), left, centerMiddle.Bottom());
			Rect3D leftCap = leftCenter;
			leftCap.pos.y = leftCap.pos.y + leftCap.size.y;
			leftCap.size.y = realBorderSize;
			leftCap.size.x += realBorderSize;
			leftCap.pos.x -= realBorderSize;
			Rect3D leftSide = leftCenter;
			leftSide.size.x = realBorderSize;
			leftSide.pos.x = leftCap.Left();

			Rect sideUv = Rect(0.0f, textureBorder, textureBorder, invTextureBorder);
			Rect capUv = Rect(0.0f, 0.0f, invTextureBorder, textureBorder); // Cap at the top
			if(swapped)
			{
				capUv = Rect(0.0f, invTextureBorder, invTextureBorder, 1.0f); // Cap at the bottom
				leftCap.size.y = realBorderSize;
				leftCap.pos.y = leftCenter.pos.y - leftCap.size.y;
			}

			MeshGenerators::GenerateSimpleXYQuad(leftCenter, centerMiddleUv, verts);
			MeshGenerators::GenerateSimpleXYQuad(leftSide, sideUv, verts);
			MeshGenerators::GenerateSimpleXYQuad(leftCap, capUv, verts);
		}

		// Generate right corner
		{
			Rect3D rightCenter = Rect3D(right, centerMiddle.Top(), right + laserWidthNoBorder, centerMiddle.Bottom());
			Rect3D rightCap = rightCenter;
			rightCap.size.y = realBorderSize;
			rightCap.size.x += realBorderSize;
			rightCap.pos.y = rightCenter.Bottom() - rightCap.size.y;
			Rect3D rightSide = rightCenter;
			rightSide.size.x = realBorderSize;
			rightSide.pos.x = rightCenter.Right();

			Rect sideUv = Rect(invTextureBorder, textureBorder, 1.0f, invTextureBorder);
			Rect capUv = Rect(textureBorder, invTextureBorder, 1.0f, 1.0f); // Cap at the bottom
			if(swapped)
			{
				capUv = Rect(textureBorder, 0.0f, 1.0f, textureBorder); // Cap at the top
				rightCap.pos.y = rightCenter.Top();
			}

			MeshGenerators::GenerateSimpleXYQuad(rightCenter, centerMiddleUv, verts);
			MeshGenerators::GenerateSimpleXYQuad(rightSide, sideUv, verts);
			MeshGenerators::GenerateSimpleXYQuad(rightCap, capUv, verts);
		}

		newMesh->SetData(verts);
		newMesh->SetPrimitiveType(PrimitiveType::TriangleList);
	}
	else
	{
		float prevLength = 0.0f;
		if(laser->prev && (laser->prev->flags & LaserObjectState::flag_Instant) != 0)
		{
			// Previous slam length
			prevLength = playback.DurationToViewDistanceAtTime(laser->prev->time, slamDuration) * laserLengthScale;
		}

		// Connecting center points
		Vector2 points[2] =
		{
			Vector2(laser->points[0] * effectiveWidth - effectiveWidth * 0.5f, prevLength), // Bottom
			Vector2(laser->points[1] * effectiveWidth - effectiveWidth * 0.5f, length * laserLengthScale), // Top
		};

		float uMin = 0.0f;
		float uMax = 1.0f;
		// More tight border in vertical direction
		float vMin = textureBorder;
		float vMax = invTextureBorder;

		float halfWidth = actualLaserWidth * 0.5f;
		Vector<MeshGenerators::SimpleVertex> verts =
		{
			{ { points[0].x - halfWidth, points[0].y,  0.0f },{ uMin, vMax } }, // BL
			{ { points[0].x + halfWidth, points[0].y,  0.0f },{ uMax, vMax } }, // BR
			{ { points[1].x + halfWidth, points[1].y,  0.0f },{ uMax, vMin } }, // TR

			{ { points[0].x - halfWidth, points[0].y,  0.0f },{ uMin, vMax } }, // BL
			{ { points[1].x + halfWidth, points[1].y,  0.0f },{ uMax, vMin } }, // TR
			{ { points[1].x - halfWidth, points[1].y,  0.0f },{ uMin, vMin } }, // TL
		};


		newMesh->SetData(verts);
		newMesh->SetPrimitiveType(PrimitiveType::TriangleList);
	}

	// Cache this mesh
	m_objectCache.Add(laser, newMesh);
	return newMesh;
}

Mesh LaserTrackBuilder::GenerateTrackEntry(class BeatmapPlayback& playback, LaserObjectState* laser)
{
	assert(laser->prev == nullptr);
	if(m_cachedEntries.Contains(laser))
		return m_cachedEntries[laser];

	Mesh newMesh = MeshRes::Create(m_gl);

	// Starting point of laser
	float startingX = laser->points[0] * effectiveWidth - effectiveWidth * 0.5f;
	
	// Length of the tail
	float length = (float)laserEntryTextureSize.y / (float)laserEntryTextureSize.x * actualLaserWidth;

	float halfWidth = actualLaserWidth * 0.5f;
	Vector<MeshGenerators::SimpleVertex> verts;
	Rect3D pos = Rect3D(Vector2(startingX - halfWidth, -length), Vector2(halfWidth * 2, length));
	Rect uv = Rect(0.0f, 0.0f, 1.0f, 1.0f);
	MeshGenerators::GenerateSimpleXYQuad(pos, uv, verts);

	newMesh->SetData(verts);
	newMesh->SetPrimitiveType(PrimitiveType::TriangleList);

	// Cache this mesh
	m_cachedEntries.Add(laser, newMesh);
	return newMesh;

}
Mesh LaserTrackBuilder::GenerateTrackExit(class BeatmapPlayback& playback, LaserObjectState* laser)
{
	assert(laser->next == nullptr);
	if(m_cachedExits.Contains(laser))
		return m_cachedExits[laser];

	Mesh newMesh = MeshRes::Create(m_gl);

	// Ending point of laser 
	float startingX = laser->points[1] * effectiveWidth - effectiveWidth * 0.5f;

	// Length of the tail
	float length = (float)laserExitTextureSize.y / (float)laserExitTextureSize.x * actualLaserWidth;

	// Length of this segment
	float prevLength = 0.0f;
	if((laser->flags & LaserObjectState::flag_Instant) != 0)
	{
		prevLength = playback.DurationToViewDistanceAtTime(laser->time, slamDuration) * laserLengthScale;
	}
	else
	{
		prevLength = playback.DurationToViewDistanceAtTime(laser->time, laser->duration) * laserLengthScale;
	}

	float halfWidth = actualLaserWidth * 0.5f;
	Vector<MeshGenerators::SimpleVertex> verts;
	Rect3D pos = Rect3D(Vector2(startingX - halfWidth, prevLength), Vector2(halfWidth * 2, length));
	Rect uv = Rect(0.0f, 0.0f, 1.0f, 1.0f);
	MeshGenerators::GenerateSimpleXYQuad(pos, uv, verts);

	newMesh->SetData(verts);
	newMesh->SetPrimitiveType(PrimitiveType::TriangleList);

	// Cache this mesh
	m_cachedExits.Add(laser, newMesh);
	return newMesh;
}

float LaserTrackBuilder::GetLaserLengthScaleAt(MapTime time)
{
	/// TODO: return scale based on speed of timing point to change horizontal laser thickness
	return 1.0f;
}

void LaserTrackBuilder::m_RecalculateConstants()
{
	// Calculate amount to scale laser size to fit the texture border in
	assert(laserTextureSize.x == laserTextureSize.y); // Use Square texture
	const float laserCenterAmount = ((float)laserTextureSize.x - ((float)laserBorderPixels * 2)) / (float)(laserTextureSize.x);
	const float laserBorderAmount = (1.0f - laserCenterAmount);

	// The uv coordinates to sample the laser without the border, or only the border
	textureBorder = (float)laserBorderPixels / laserTextureSize.x;
	invTextureBorder = 1.0f - textureBorder;

	// The the size of the laser with compensation added for the border
	actualLaserWidth = m_laserWidth / laserCenterAmount;

	// The width of the laser without the border
	laserWidthNoBorder = actualLaserWidth * laserCenterAmount;
	// World size of a single border around a laser
	realBorderSize = (actualLaserWidth - laserWidthNoBorder) * 0.5f;

	// The length of the horizontal slam segments
	slamDuration = 40;

	// The effective area in which the center point of the laser can move
	effectiveWidth = m_trackWidth - m_laserWidth;
}

void LaserTrackBuilder::m_Cleanup(MapTime newTime, Map<LaserObjectState*, Mesh>& arr)
{
	// Cleanup unused meshes
	for(auto it = arr.begin(); it != arr.end();)
	{
		LaserObjectState* obj = it->first;
		MapTime endTime = obj->time + obj->duration + 1000;
		if(newTime > endTime)
		{
			it = arr.erase(it);
			continue;
		}
		it++;
	}
}
void LaserTrackBuilder::Reset()
{
	m_objectCache.clear();
	m_cachedEntries.clear();
	m_cachedExits.clear();
	m_RecalculateConstants();
}
void LaserTrackBuilder::Update(MapTime newTime)
{
	m_Cleanup(newTime, m_objectCache);
	m_Cleanup(newTime, m_cachedEntries);
	m_Cleanup(newTime, m_cachedExits);
}