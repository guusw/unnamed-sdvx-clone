#pragma once
#include <list>

/* 
	Doubly linked list
	wrapper around std::list
*/
template<typename I>
class List : public std::list<I>
{
public:
	using std::list<I>::list;
	using std::list<I>::list::insert;
	using std::list<I>::list::begin;
	using std::list<I>::list::end;
	using std::list<I>::list::sort;

	I& AddBack(const I& item = I())
	{
		auto it = insert(end(), item);
		return *it;
	}
	I& AddFront(const I& item = I())
	{
		auto it = insert(begin(), item);
		return *it;
	}

	// Sort function with uppercase for consistency
	template<typename Predicate>
	void Sort(Predicate& pred)
	{
		sort(pred);
	}
};