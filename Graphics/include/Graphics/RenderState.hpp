#pragma once

namespace Graphics
{
	/*
		Contains all values that get passed as built-in values to material shaders
	*/
	class RenderState
	{
	public:
		static RenderState Create2DRenderState(Recti viewport);

	public:
		Transform worldTransform;
		Transform projectionTransform;
		Transform cameraTransform;
		Vector2i viewportSize;
		float aspectRatio;
		float time;

	};
}