#pragma once

// Data passed to GUI element to draw
// contains allocated area, timing info, and various rendering stuff
struct GUIRenderData
{
	// GUI Renderer class
	class GUIRenderer* guiRenderer;
	// Queue to draw to
	class RenderQueue* rq;
	// Render transform
	Transform2D transform;
	// Assigned render area
	Rect area;
	// If size is fixed
	bool isSizeFixed = false;
	// Time between this and last render call
	float deltaTime;
	// Should render debug shapes
	bool debug = true;

	// Use this overlap test for correct clipping rectangle tests
	bool OverlapTest(Rect rect) const;
};