#pragma once

namespace Graphics
{
	class RenderQueue;
}

/*
	Data passed to GUI element to draw
	contains allocated area, timing info, and various rendering stuff
*/
struct GUIRenderData
{
	// GUI Renderer class
	class GUIRenderer* guiRenderer;
	// Queue to draw to
	Graphics::RenderQueue* rq;
	// Should render debug shapes
	bool debug = true;
};