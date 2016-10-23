#include "stdafx.h"
#include "Transform2D.hpp"
#include "Rect.hpp"
#include <cmath>

Transform2D::Transform2D(Vector2 translation, Vector2 scale /*= Vector2(1.0f, 1.0f)*/)
{
	mat[6] = translation.x;
	mat[7] = translation.y;

	mat[0] = scale.x;
	mat[4] = scale.y;
}
Transform2D::Transform2D(Vector2 translation, Vector2 scale, float rotation)
{
	mat[6] = translation.x;
	mat[7] = translation.y;

	rotation *= Math::degToRad;
	mat[0] = cos(rotation) * scale.x;
	mat[1] = -sin(rotation) * scale.x;
	mat[3] = sin(rotation) * scale.y;
	mat[4] = cos(rotation)  * scale.y;
}
Transform2D::Transform2D(const Matrix3x3& mat)
{
	*(Matrix3x3*)this = mat;
}
void Transform2D::ScaleTransform(const Vector2& scale)
{
	Transform2D factor;
	factor[0] = scale.x;
	factor[4] = scale.y;
	*this *= factor;
}
void Transform2D::SetIdentity()
{
	Utility::MemsetZero(mat);
	mat[0] = 1.0f;
	mat[4] = 1.0f;
	mat[8] = 1.0f;
}
Transform2D Transform2D::Translation(const Vector2& pos)
{
	Transform2D ret;
	ret.mat[6] = pos.x;
	ret.mat[7] = pos.y;
	return ret;
}
Transform2D Transform2D::Rotation(float rotation)
{
	rotation *= Math::degToRad;

	Transform2D ret; 
	ret.mat[0] = cos(rotation);
	ret.mat[1] = -sin(rotation);
	ret.mat[3] = sin(rotation);
	ret.mat[4] = cos(rotation);
	return ret;
}
Transform2D Transform2D::RotationAround(float rotation, Vector2 pivot)
{
	return Translation(pivot) * Rotation(rotation) * Translation(-pivot);
}

Transform2D Transform2D::Scale(const Vector2& scale)
{
	Transform2D ret;
	ret.mat[0] = scale.x;
	ret.mat[4] = scale.y;
	return ret;
}
Vector2 Transform2D::GetPosition() const
{
	return Vector2(this->mat[6], this->mat[7]);
}
Vector2 Transform2D::GetScale() const
{
	Vector2& x = *(Vector2*)mat;
	Vector2& y = *(Vector2*)(mat+3);
	return Vector2(x.Length(), y.Length());
}
float Transform2D::GetRotation() const
{
	Vector2& y = *(Vector2*)(mat + 3);
	Vector2 ynorm = y.Normalized();
	return atan2f(ynorm.y, ynorm.x);
}
Vector2 Transform2D::GetUp() const
{
	Vector2& y = *(Vector2*)(mat + 3);
	return y.Normalized();
}
Vector2 Transform2D::GetRight() const
{
	Vector2& x = *(Vector2*)(mat + 0);
	return x.Normalized();
}

Vector2 Transform2D::TransformPoint(const Vector2& position) const
{
	const float w = this->mat[2] * position.x + this->mat[5] * position.y + this->mat[8];
	return Vector2(
		this->mat[0] * position.x + this->mat[3] * position.y + this->mat[6],
		this->mat[1] * position.x + this->mat[4] * position.y + this->mat[7]) / w;
}
Vector2 Transform2D::TransformDirection(const Vector2& direction) const
{
	return Vector2(
		this->mat[0] * direction.x + this->mat[3] * direction.y,
		this->mat[1] * direction.x + this->mat[4] * direction.y);
}