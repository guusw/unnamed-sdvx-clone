#pragma once
#include <unordered_map>

/* Unordered Map class, extends std::map */
template<typename K, typename V, typename Hash = std::hash<K>>
class UnorderedMap : public std::unordered_map<K, V, Hash>
{
public:
	using std::unordered_map<K, V, Hash>::unordered_map;

	// These are for allowing function to be called on the base class when compiling on GCC
	using std::unordered_map<K, V, Hash>::begin;
	using std::unordered_map<K, V, Hash>::end;
	using std::unordered_map<K, V, Hash>::erase;
	using std::unordered_map<K, V, Hash>::insert;
	using std::unordered_map<K, V, Hash>::find;

	bool Contains(const K& key) const
	{
		return find(key) != end();
	}
	V& FindOrAdd(const K& key, const V& defaultValue = V())
	{
		auto it = find(key);
		if(it == end())
		{
			it = insert(std::make_pair(key, defaultValue)).first;
		}
		return it->second;
	}
	V& Add(const K& k, const V& v = V())
	{
		return FindOrAdd(k) = v;
	}

	// Finds the value in the map or returns null
	V* Find(const K& k)
	{
		auto it = find(k);
		if(it == end())
			return nullptr;
		return &it->second;
	}
	const V* Find(const K& k) const
	{
		auto it = find(k);
		if(it == end())
			return nullptr;
		return &it->second;
	}
};
