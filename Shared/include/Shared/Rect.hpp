#pragma once
#include "Shared/VectorMath.hpp"
#include "Shared/Transform2D.hpp"

/*
	GUI space rectangle with bottom as y+height
*/
template<typename T>
class RectangleBase
{
public:
	typedef VectorMath::VectorBase<T, 2> VectorType;
	VectorType pos;
	VectorType size;
	// Conversion from other types
	template<typename Q>
	RectangleBase(const RectangleBase<Q>& other)
	{
		pos = (VectorType)other.pos;
		size = (VectorType)other.size;
	}
	// Give all 4 sides of the rectangle
	RectangleBase(T all = 0) : RectangleBase(all, all, all, all) {}
	RectangleBase(T left, T top, T right, T bottom)
	{
		pos = VectorType(left, top);
		size = VectorType(right - left, bottom - top);
	}
	RectangleBase(const VectorType& pos, const VectorType& size)
		: pos(pos), size(size)
	{
	}

	bool operator==(const RectangleBase& other) const
	{
		return pos == other.pos && size == other.size;
	}
	bool operator!=(const RectangleBase& other) const
	{
		return pos != other.pos || size != other.size;
	}

	T Left() const
	{
		return pos.x;
	}
	T Top() const
	{
		return pos.y;
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
	// Moves the upper and lower edges of this rectangle inward or outward
	RectangleBase OffsetY(float amount) const
	{
		RectangleBase newRect = *this;
		VectorType newSize = newRect.size + VectorType(0, amount);
		if(newSize.y < 0)
			newSize.y = 0;

		VectorType halfDelta = (newSize - newRect.size);
		newRect.pos -= halfDelta;
		newRect.size += halfDelta * 2;
		return newRect;
	}
	// Moves the upper and lower edges of this rectangle inward or outward
	RectangleBase OffsetX(float amount) const
	{
		RectangleBase newRect = *this;
		VectorType newSize = newRect.size + VectorType(amount, 0);
		if(newSize.x < 0)
			newSize.x = 0;

		VectorType halfDelta = (newSize - newRect.size);
		newRect.pos -= halfDelta;
		newRect.size += halfDelta * 2;
		return newRect;
	}

	// Clamp the parameter to this rectangle
	RectangleBase Clamp(const RectangleBase& other) const
	{
		float top = Math::Max(other.Top(), Top());
		float bottom = Math::Min(other.Bottom(), Bottom());
		float left = Math::Max(other.Left(), Left());
		float right = Math::Min(other.Right(), Right());
		if(right < left)
			right = left;
		if(bottom < top)
			bottom = top;
		return RectangleBase(left, top, right, bottom);
	}

	// Converts this rectangle (pos,size) to a 2D transformation matrix
	Transform2D ToTransform() const
	{
		return Transform2D(pos, size);
	}

	// The center of this rectangle
	VectorType Center() const
	{
		return pos + size * 0.5f;
	}

	// Expand this bounding rectangle to include the given point
	void Expand(const Vector2& point)
	{
		if(point.x < pos.x)
		{
			size.x += pos.x - point.x;
			pos.x = point.x;
		}
		if(point.y < pos.y)
		{
			size.y += pos.y - point.y;
			pos.y = point.y;
		}

		Vector2 sd = point - pos;
		if(sd.x > size.x)
			size.x = sd.x;
		if(sd.y > size.y)
			size.y = sd.y;
	}
	// Expand this bounding rectangle to include the given rectangle
	void Expand(const RectangleBase& other)
	{
		Expand(other.pos);
		Expand(other.pos + VectorType(other.size.x, 0));
		Expand(other.pos + VectorType(0, other.size.y));
		Expand(other.pos + VectorType(other.size.x, 1));
	}
	// Expand this bounding rectangle to include the given rectangle transformed by transform
	void Expand(const RectangleBase& other, Transform2D transform)
	{
		Expand(transform.TransformPoint(other.pos));
		Expand(transform.TransformPoint(other.pos + VectorType(other.size.x, 0)));
		Expand(transform.TransformPoint(other.pos + VectorType(0, other.size.y)));
		Expand(transform.TransformPoint(other.pos + VectorType(other.size.x, 1)));
	}

	bool ContainsPoint(const VectorType& point) const
	{
		if(point.x < pos.x)
			return false;
		if(point.y < pos.y)
			return false;
		if(point.x > (pos.x + size.x))
			return false;
		if(point.y > (pos.y + size.y))
			return false;
		return true;
	}

	// Initial value for bounding rectangles
	static RectangleBase Empty;
};

template<typename T>
RectangleBase<T> RectangleBase<T>::Empty = RectangleBase<T>(Vector2(FLT_MAX * 0.5f), Vector2(-FLT_MAX));

/* 
	Same as a normal rectangle but this one has the top as y+height aka world space
*/
template<typename T>
class RectangleBase3D : public RectangleBase<T>
{
public:
	typedef VectorMath::VectorBase<T, 2> VectorType;
	using RectangleBase<T>::RectangleBase;
	using RectangleBase<T>::RectangleBase::pos;
	using RectangleBase<T>::RectangleBase::size;
	
	RectangleBase3D() = default;
	// Conversion from other types
	template<typename Q>
	RectangleBase3D(const RectangleBase<Q>& other)
	{
		pos = (VectorType)other.pos;
		size = (VectorType)other.size;
	}
	RectangleBase3D(const RectangleBase<T>& other)
	{
		pos = other.pos; 
		size = other.size;
	};
	RectangleBase3D(T left, T top, T right, T bottom)
	{
		pos = VectorType(left, bottom);
		size = VectorType(right - left, top - bottom);
	}
	T Top() const
	{
		return pos.y + size.y;
	}
	T Bottom() const
	{
		return pos.y;
	}
};

typedef RectangleBase<float> Rect;
typedef RectangleBase<int32> Recti;
typedef RectangleBase3D<float> Rect3D;
typedef RectangleBase3D<int32> Recti3D;