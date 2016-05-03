#pragma once
#include "RenderState.hpp"

/*
	Camera that hovers above the playfield track and can process camera shake and tilt effects
*/
class Camera
{
public:
	// Generates a new render state for drawing from this cameras Point of View
	// the clipped boolean indicates whenether to clip the cameras clipping planes to the track range
	RenderState GenerateRenderState(bool clipped);

	// The track being watched
	class Track* track;
};