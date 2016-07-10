#include "stdafx.h"
#include "Config.hpp"
#include "TextStream.hpp"
#include <sstream>

bool Config::Load(BinaryStream& stream, bool binary /*= false*/)
{
	Clear();
	if(binary)
	{
		stream << m_entries;
	}
	else
	{
		String line;
		while(TextStream::ReadLine(stream, line))
		{
			String k, v;
			if(line.Split("=", &k, &v))
			{
				k.Trim(' ');
				v.Trim(' ');

				// Detect type of right hand side
				std::stringstream s(v);

				int32 i;
				s >> i;
				if(!s.fail() && s.eof())
				{
					Add(k, Variant::Create(i));
					continue;
				}
				s.seekg(0);
				s.clear();
				float f;
				s >> f;
				if(!s.fail() && s.eof())
				{
					Add(k, Variant::Create(f));
					continue;
				}

				// Trim quotes
				v.Trim('"');
				Add(k, Variant::Create(v));
			}
		}
	}

	return true;
}
void Config::Save(BinaryStream& stream, bool binary /*= false*/)
{
	if(binary)
	{
		stream << m_entries;
	}
	else
	{
		for(auto& p : m_entries)
		{
			String v = p.second.ToString();
			if(p.second.GetType() == VariantType::String)
				v = "\"" + v + "\"";
			String line = p.first + " = " + v;
			TextStream::WriteLine(stream, line);
		}
	}
}

Variant* Config::Get(const String& key)
{
	return m_entries.Find(key);
}
void Config::Add(const String& key, const Variant& val)
{
	m_entries.Add(key, val);
}
bool Config::Contains(const String& key) const
{
	return m_entries.Contains(key);
}
void Config::Clear()
{
	m_entries.clear();
}
