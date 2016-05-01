#include "stdafx.h"
#include "LaserTrackBuilder.hpp"
#include "MeshGenerators.hpp"
#include "BeatmapPlayback.hpp"
#include <algorithm>

void GenerateLaserQuad(Rect3D r, Rect uv, Vector<MeshGenerators::SimpleVertex>& out)
{
	Vector<MeshGenerators::SimpleVertex> verts =
	{
		{ { r.Left(),  r.Top(),     0.0f },{ uv.Left(), uv.Top() } },
		{ { r.Right(), r.Bottom(),  0.0f },{ uv.Right(), uv.Bottom() } },
		{ { r.Right(), r.Top(),     0.0f },{ uv.Right(), uv.Top() } },

		{ { r.Left(),  r.Top(),     0.0f },{ uv.Left(), uv.Top() } },
		{ { r.Left(),  r.Bottom(),  0.0f },{ uv.Left(), uv.Bottom() } },
		{ { r.Right(), r.Bottom(),  0.0f },{ uv.Right(), uv.Bottom() } },
	};
	for(auto& v : verts)
		out.Add(v);
}

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
	float laserCenterAmount = ((float)laserTextureSize.x - ((float)laserBorderPixels*2)) / (float)(laserTextureSize.x);
	float laserBorderAmount = (1.0f - laserCenterAmount);

	// The uv coordinates to sample the laser without the border, or only the border
	float textureBorder = (float)laserBorderPixels / laserTextureSize.x;
	float invTextureBorder = 1.0f - textureBorder;

	// The width of the laser without the border
	float laserWidthNoBorder = m_laserWidth * laserCenterAmount;
	// World size of a single border around a laser
	float realBorderSize = (m_laserWidth - laserWidthNoBorder) * 0.5f;

	float effectiveWidth = m_trackWidth - m_laserWidth;
	float length = playback.DurationToBarDistance(laser->duration);

	if((laser->flags & LaserObjectState::flag_Instant) != 0) // Slam segment
	{
		float left, right;
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
			left += laserWidthNoBorder * 0.5f;
		else // otherwise extend all the way from the center to the edge
			left -= laserWidthNoBorder * 0.5f;
		if(cornerRight)
			right -= laserWidthNoBorder * 0.5f;
		else
			right += laserWidthNoBorder * 0.5f;

		// Uv coordinates for center and borders
		// More tight border in horizontal direction
		Rect centerMiddleUv = Rect(textureBorder, textureBorder, invTextureBorder, invTextureBorder);
		Rect centerUpperUv = Rect(textureBorder, 0.0f, invTextureBorder, textureBorder);
		Rect centerLowerUv = Rect(textureBorder, invTextureBorder, invTextureBorder, 1.0f);

		// Generate positions for middle top and bottom
		Rect3D centerMiddle = Rect3D(left, laserWidthNoBorder * perspectiveHeightScale, right, 0.0f);
		Rect3D centerBottom = centerMiddle;
		centerBottom.size.y = realBorderSize * perspectiveHeightScale;
		centerBottom.pos.y = centerMiddle.Bottom() - centerBottom.size.y;
		Rect3D centerTop = centerBottom;
		centerTop.pos.y = centerMiddle.Top();

		float uvMin = textureBorder;
		float uvMax = invTextureBorder;

		Vector<MeshGenerators::SimpleVertex> verts;

		// Middle part
		GenerateLaserQuad(centerTop, centerUpperUv, verts);
		GenerateLaserQuad(centerBottom, centerLowerUv, verts);
		GenerateLaserQuad(centerMiddle, centerMiddleUv, verts);

		// Generate left corner
		if(cornerLeft)
		{
			Rect3D leftCenter = Rect3D(left - laserWidthNoBorder, centerMiddle.Top(), left, centerMiddle.Bottom());
			Rect3D leftCap = leftCenter;
			leftCap.pos.y = leftCap.pos.y + leftCap.size.y;
			leftCap.size.y = realBorderSize * perspectiveHeightScale;
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
				leftCap.size.y = realBorderSize * perspectiveHeightScale;
				leftCap.pos.y = leftCenter.pos.y - leftCap.size.y;
			}

			GenerateLaserQuad(leftCenter, centerMiddleUv, verts);
			GenerateLaserQuad(leftSide, sideUv, verts);
			GenerateLaserQuad(leftCap, capUv, verts);
		}

		if(cornerRight)
		{
			Rect3D rightCenter = Rect3D(right, centerMiddle.Top(), right + laserWidthNoBorder, centerMiddle.Bottom());
			Rect3D rightCap = rightCenter;
			rightCap.size.y = realBorderSize * perspectiveHeightScale;
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

			GenerateLaserQuad(rightCenter, centerMiddleUv, verts);
			GenerateLaserQuad(rightSide, sideUv, verts);
			GenerateLaserQuad(rightCap, capUv, verts);
		}

		newMesh->SetData(verts);
		newMesh->SetPrimitiveType(PrimitiveType::TriangleList);
	}
	else
	{
		float prevLength = 0.0f;
		if(laser->prev && (laser->prev->flags & LaserObjectState::flag_Instant) != 0)
		{
			prevLength = laserWidthNoBorder * perspectiveHeightScale;
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

		float halfWidth = m_laserWidth * 0.5f;
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