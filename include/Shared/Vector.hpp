#pragma once
#include <vector>

/* Vector class, extends std::vector */
template<typename I>
class Vector : public std::vector<I>
{
public:
	using std::vector<I>::vector;
	// Adds a new element and returns it
	I& Add(const I& obj = T()) { push_back(obj); return back(); };
	I& AddZeroed() { push_back(I()); memset(&back(), 0, sizeof(I)); return back(); };

	// O(N) lookup to check if object is in array
	bool Contains(const I& obj) const 
	{
		for(auto& i : *this)
		{
			if(obj == i)
				return true;
		}
		return false;
	}
};