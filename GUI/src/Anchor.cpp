#include "stdafx.h"
#include "Anchor.hpp"

Vector2 Anchor::GetAreaMultiplier() const
{
	Vector2 delta = Delta();
	return Vector2((delta.x > 0) ? 1.0f : 0.0f, (delta.y > 0) ? 1.0f : 0.0f);
}
Vector2& Anchor::TopLeft()
{
	return *reinterpret_cast<Vector2*>(&left);
}
const Vector2& Anchor::TopLeft() const
{
	return *reinterpret_cast<const Vector2*>(&left);
}

Vector2& Anchor::BottomRight()
{
	return *reinterpret_cast<Vector2*>(&right);
}
const Vector2& Anchor::BottomRight() const
{
	return *reinterpret_cast<const Vector2*>(&right);
}
Vector2 Anchor::Delta() const
{
	return Vector2(right - left, bottom - top);
}

const Anchor Anchors::TopLeft = Anchor(0.0f, 0.0f);
const Anchor Anchors::TopMiddle = Anchor(0.5f, 0.0f);
const Anchor Anchors::TopsRight = Anchor(1.0f, 0.0f);
const Anchor Anchors::MiddleLeft = Anchor(0.0f, 0.5f);
const Anchor Anchors::Middle = Anchor(0.5f, 0.5f);
const Anchor Anchors::MiddleRight = Anchor(1.0f, 0.5f);
const Anchor Anchors::BottomLeft = Anchor(0.0f, 1.0f);
const Anchor Anchors::BottomMiddle = Anchor(0.5f, 1.0f);
const Anchor Anchors::BottomRight = Anchor(1.0f, 1.0f);
const Anchor Anchors::Full = Anchor(0, 0, 1, 1);