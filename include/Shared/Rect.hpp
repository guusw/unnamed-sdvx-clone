#pragma once
#include "VectorMath.hpp"

template<typename T>
class RectangleBase
{
public:
	typedef VectorMath::VectorBase<T, 2> VectorType;
	VectorType pos;
	VectorType size;
	RectangleBase() = default;
	RectangleBase(T left, T top, T right, T bottom)
	{
		pos = VectorType(left, top);
		size = VectorType(right - left, bottom - top);
	}
	RectangleBase(const VectorType& pos, const VectorType& size)
		: pos(pos), size(size)
	{
	}
	T Bottom() const
	{
		return pos.y + size.y;
	}
	T Right() const
	{
		return pos.x + size.x;
	}
	// Moves the edges of this rectangle inward or outward
	RectangleBase Offset(float amount) const
	{
		RectangleBase newRect = *this;
		VectorType newSize = newRect.size + VectorType(amount);
		if(newSize.x < 0)
			newSize.x = 0;
		if(newSize.y < 0)
			newSize.y = 0;

		VectorType halfDelta = (newSize - newRect.size);
		newRect.pos -= halfDelta;
		newRect.size += halfDelta * 2;
		return newRect;
	}
};

typedef RectangleBase<float> Rect;
typedef RectangleBase<int32> Recti;