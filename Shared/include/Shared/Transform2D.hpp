#pragma once
#include "Shared/Utility.hpp"
#include "Shared/Math.hpp"
#include "Shared/VectorMath.hpp"
#include <initializer_list>

/*
	3x3 Transformation matrix class used in 2D rendering
	Column-Major order, Left handed
*/
class Transform2D
{
public:
	Transform2D();
	Transform2D(Vector2 translation, Vector2 scale = Vector2(1.0f, 1.0f));
	Transform2D(Vector2 translation, Vector2 scale, float rotation);
	Transform2D(const Transform2D& other);
	Transform2D(std::initializer_list<float> values);
	Transform2D& operator=(const Transform2D& right);

	const float& operator[](size_t idx) const;
	float& operator[](size_t idx);

	Transform2D operator*(const Transform2D& other) const;
	Transform2D& operator*=(const Transform2D& other);

	void ScaleTransform(const Vector2& scale);

	void SetIdentity();

	static Transform2D Translation(const Vector2& pos);
	static Transform2D Rotation(float rotation);
	static Transform2D RotateAround(float rotation, Vector2 pivot);
	static Transform2D Scale(const Vector2& scale);

	Vector2 GetPosition() const;
	Vector2 GetScale() const;
	float GetRotation() const;

	Vector2 GetUp() const;
	Vector2 GetRight() const;

	Vector2 TransformPoint(const Vector2& position) const;
	Vector2 TransformDirection(const Vector2& direction) const;

	float mat[9] = 
	{
		1, 0, 0,
		0, 1, 0,
		0, 0, 1,
	};
};