#pragma once

// Data passed to GUI element to draw
// contains allocated area, timing info, and various rendering stuff
struct GUIRenderData
{
	// GUI Renderer class
	class GUIRenderer* guiRenderer;
	// Queue to draw to
	RenderQueue* rq;
	// Transform for elements
	Transform transform;
	// Assigned render area
	Rect area;
	// If size is fixed
	bool isSizeFixed = false;
	// Time between this and last render call
	float deltaTime;

	// Use this overlap test for correct clipping rectangle tests
	bool OverlapTest(Rect rect) const;
};