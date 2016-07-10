#pragma once
#include "Shared/Variant.hpp"
#include "Shared/Map.hpp"
#include "BinaryStream.hpp"

/*
	Config file class
	Used to save and load data to a text file format
*/
class Config
{
	Map<String, Variant> m_entries;
public:
	bool Load(BinaryStream& stream, bool binary = false);
	void Save(BinaryStream& stream, bool binary = false);

	Variant* Get(const String& key);
	void Add(const String& key, const Variant& val);
	bool Contains(const String& key) const;
	void Clear();
};