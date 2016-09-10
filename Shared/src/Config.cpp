#include "stdafx.h"
#include "Config.hpp"
#include "TextStream.hpp"
#include "Set.hpp"
#include <sstream>

ConfigBase::~ConfigBase()
{
	for(auto e : m_entries)
	{
		delete e.second;
	}
}
bool ConfigBase::Load(BinaryStream& stream)
{
	// Clear and load defaults
	Clear();

	Set<uint32> setKeys;
	for(auto e : m_entries)
	{
		setKeys.Add(e.first);
	}

	String line;
	while(TextStream::ReadLine(stream, line))
	{
		String k, v;
		if(line.Split("=", &k, &v))
		{
			k.Trim(' ');
			v.Trim(' ');
			std::stringstream s(v);

			auto it = m_keys.find(k);
			if(it != m_keys.end())
			{
				auto it1 = m_entries.find(it->second);
				if(it1 != m_entries.end())
				{
					setKeys.erase(it1->first);
					m_entries[it1->first]->FromString(v);
				}
			}
		}
	}


	if(!setKeys.empty())
	{
		// Default setting missed in config file, flag as dirty
		m_dirty = true;
	}
	else
	{
		// Config is clean
		m_dirty = false;
	}
	return true;
}
void ConfigBase::Save(BinaryStream& stream)
{
	for(auto& e : m_entries)
	{
		String key = m_reverseKeys[e.first];
		String line = key + " = " + e.second->ToString();
		TextStream::WriteLine(stream, line);
	}

	// Saved
	m_dirty = false;
}
bool ConfigBase::IsDirty() const
{
	return m_dirty;
}
void ConfigBase::Clear()
{
	for(auto e : m_entries)
	{
		delete e.second;
	}
	m_entries.clear();
	InitDefaults();
}
ConfigBase::ConfigBase()
{
}