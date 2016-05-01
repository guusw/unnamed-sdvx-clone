#pragma once

class RenderState
{
public:
	Transform worldTransform;
	Transform projectionTransform;
	Transform cameraTransform;
	Vector2i viewportSize;
	float aspectRatio;
};