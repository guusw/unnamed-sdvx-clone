#pragma once

/*
	Represents Top-Left and Bottom-Right anchors inside a rectangle in [0,1] range
	An anchor can (per axis) either be a point anchor, e.g. left==right = 0.5
	or an area anchor, e.g. left=0.2 right=0.8
*/
class Anchor
{
public:
	Anchor(float all = 0.0f) : left(all), top(all), right(all), bottom(all)
	{
	}
	// Area anchor
	Anchor(float left, float top, float right, float bottom)
		: left(left), top(top), right(right), bottom(bottom) 
	{
	}
	// Point anchor
	Anchor(float x, float y)
		: left(x), top(y), right(x), bottom(y)
	{
	}

	// Apply anchor to an input rectangle
	Rect Apply(const Rect& in)
	{
		Vector2 delta = Delta();
		Vector2 topLeft = in.pos + in.size * TopLeft();
		Vector2 bottomRight = in.pos + in.size * BottomRight();
		return Rect(topLeft, bottomRight-topLeft);
	}

	Vector2 GetAreaMultiplier() const;

	Vector2& TopLeft();
	const Vector2& TopLeft() const;

	Vector2& BottomRight();
	const Vector2& BottomRight() const;

	// (Right-Left, Bottom-Top) Difference vector
	Vector2 Delta() const;

	float left;
	float top;
	float right;
	float bottom;
};

namespace Anchors
{
	extern const Anchor TopLeft;
	extern const Anchor TopMiddle;
	extern const Anchor TopsRight;
	extern const Anchor MiddleLeft;
	extern const Anchor Middle;
	extern const Anchor MiddleRight;
	extern const Anchor BottomLeft;
	extern const Anchor BottomMiddle;
	extern const Anchor BottomRight;
	extern const Anchor Full;
}