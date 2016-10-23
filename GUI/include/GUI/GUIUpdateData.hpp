#pragma once

/*
	Data passed to a GUI element to build their final position on the screen
	this kind of works like a stack in that the element modifies 
	the area and transform field and passes this data to it's children
*/
struct GUIUpdateData
{
	// Assigned render area
	Rect area;
	// Time between this and last render call
	float deltaTime;
	// Should render debug shapes
	bool debug = true;
	// Render transform stack
	Transform2D renderTransform;
};