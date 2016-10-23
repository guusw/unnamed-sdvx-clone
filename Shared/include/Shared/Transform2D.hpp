///////////////////////////////////////////////////////////////////////////////////////////////////////////////
//                                                                                                           //
//  Copyright (c) 2012-2015, Jan de Graaf (jan@jlib.nl)                                                      //
//                                                                                                           //
//  Permission to use, copy, modify, and/or distribute this software for any purpose with or                 //
//  without fee is hereby granted, provided that the above copyright notice and this permission              //
//  notice appear in all copies.                                                                             //
//                                                                                                           //
//  THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES WITH REGARD TO THIS             //
//  SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL              //
//  THE AUTHOR BE LIABLE FOR ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES          //
//  WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE     //
//  OR OTHER TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.  //
//                                                                                                           //
///////////////////////////////////////////////////////////////////////////////////////////////////////////////

// Modified version of 
//	(http://jlib.nl/index.php?file=./code/include/jlib/transform)

#pragma once
#include "Shared/Matrix.hpp"

/*
	3x3 Transformation matrix class used in 2D rendering
	Column-Major order, Left handed
*/
class Transform2D : public Matrix3x3
{
public:
	using Matrix3x3::Matrix;
	using Matrix3x3::mat;

	Transform2D() = default;
	Transform2D(const Matrix3x3& mat);
	Transform2D(Vector2 translation, Vector2 scale = Vector2(1.0f, 1.0f));
	Transform2D(Vector2 translation, Vector2 scale, float rotation);

	// Scale this transformation with a scaling vector
	void ScaleTransform(const Vector2& scale);

	// Reset to identity
	void SetIdentity();

	static Transform2D Translation(const Vector2& pos);
	static Transform2D Rotation(float rotation);
	static Transform2D RotationAround(float rotation, Vector2 pivot);
	static Transform2D Scale(const Vector2& scale);

	Vector2 GetPosition() const;
	Vector2 GetScale() const;
	float GetRotation() const;

	Vector2 GetUp() const;
	Vector2 GetRight() const;

	Vector2 TransformPoint(const Vector2& position) const;
	Vector2 TransformDirection(const Vector2& direction) const;
};