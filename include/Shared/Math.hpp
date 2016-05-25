#pragma once
#include <math.h>

namespace Math
{
	// Floating point PI constant
	extern const float pi;
	extern const float degToRad;
	extern const float radToDeg;

	// Templated min
	template<typename T>
	static T Min(T a, T b)
	{
		if(a < b)
			return a;
		else
			return b;
	}

	// Templated max
	template<typename T>
	static T Max(T a, T b)
	{
		if(a > b)
			return a;
		else
			return b;
	}

	template<typename T>
	static T Clamp(T v, T min, T max)
	{
		if(v < min)
			return min;
		if(v > max)
			return max;
		return v;
	}

	// Templated Greatest common divisor
	template<typename T>
	static T GCD(T a, T b)
	{
		return b == 0 ? a : gcd(b, a % b);
	}

	// Gets the sign of a value
	template <typename T> T Sign(T val) {
		return (T)((T(0) < val) - (val < T(0)));
	}

	// Returns angular difference between 2 angles (radians)
	// closest path
	float AngularDifference(float a, float b);
}
