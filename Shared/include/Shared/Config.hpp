#pragma once
#include "Shared/ConfigEntry.hpp"
#include "Shared/Map.hpp"
#include "Shared/Enum.hpp"
#include "Shared/Unique.hpp"
#include "Shared/BinaryStream.hpp"

/*
	Base class used for config files
	Used to save and load data to a text file format
	The config file uses an enum key to index the config file members
*/
class ConfigBase : public Unique
{
public:
	virtual ~ConfigBase();

	// Load from text file
	bool Load(BinaryStream& stream);
	// Save to text file
	void Save(BinaryStream& stream);

	bool IsDirty() const;

	// Resets config back to default state
	void Clear();

protected:
	ConfigBase();
	virtual void InitDefaults() = 0;

	Map<String, uint32> m_keys;
	Map<uint32, String> m_reverseKeys;
	Map<uint32, IConfigEntry*> m_entries;
	bool m_dirty = false;
};

/* 
	Template config class, specify the type Enum_<enum name> as the key enum for config entries
*/
template<typename EnumClass>
class Config : public ConfigBase
{
public:
	typedef typename EnumClass::EnumType KeyType;
	Config()
	{
		for(auto it : EnumClass::GetMap())
		{
			m_keys.Add(it.second, (uint32)it.first);
			m_reverseKeys.Add((uint32)it.first, it.second);
		}
	}
	bool IsSet(KeyType key) const
	{
		auto it = m_entries.find((uint32)key);
		return it != m_entries.end();
	}

	template<typename T> T Get(KeyType key) const;
	template<> int32 Get<int32>(KeyType key) const
	{
		return GetEnsure<IntConfigEntry>(key)->data;
	}
	template<> float Get<float>(KeyType key) const
	{
		return GetEnsure<FloatConfigEntry>(key)->data;
	}
	template<> String Get<String>(KeyType key)const
	{
		return GetEnsure<StringConfigEntry>(key)->data;
	}
	template<> bool Get<bool>(KeyType key)const
	{
		return GetEnsure<BoolConfigEntry>(key)->data;
	}
	template<typename EnumClass>
	typename EnumClass::EnumType GetEnum(KeyType key) const
	{
		return GetEnsure<EnumConfigEntry<EnumClass>>(key)->data;
	}

	void Set(KeyType key, const int32& value)
	{
		int32& dst = SetEnsure<IntConfigEntry>(key)->data;
		if(dst != value)
		{
			dst = value;
			m_dirty = true;
		}
	}
	void Set(KeyType key, const float& value)
	{
		float& dst = SetEnsure<FloatConfigEntry>(key)->data;
		if(dst != value)
		{
			dst = value;
			m_dirty = true;
		}
	}
	void Set(KeyType key, const bool& value)
	{
		bool& dst = SetEnsure<BoolConfigEntry>(key)->data;
		if(dst != value)
		{
			dst = value;
			m_dirty = true;
		}
	}
	void Set(KeyType key, const String& value)
	{
		String& dst = SetEnsure<StringConfigEntry>(key)->data;
		if(dst != value)
		{
			dst = value;
			m_dirty = true;
		}
	}
	void Set(KeyType key, const char* value)
	{
		String& dst = SetEnsure<StringConfigEntry>(key)->data;
		if(dst != value)
		{
			dst = value;
			m_dirty = true;
		}
	}
	template<typename EnumClass>
	void SetEnum(KeyType key, typename EnumClass::EnumType value)
	{
		typename EnumClass::EnumType& dst = SetEnsure<EnumConfigEntry<EnumClass>>(key)->data;
		if(dst != value)
		{
			dst = value;
			m_dirty = true;
		}
	}

private:
	// Create or returns with type checking
	template<typename T> T* SetEnsure(KeyType key)
	{
		IConfigEntry** found = m_entries.Find((uint32)key);
		if(found)
		{
			T* targetType = found[0]->As<T>();
			assert(targetType); // Make sure type matches
			return targetType;
		}
		else
		{
			// Add new entry
			T* ret = new T();
			m_entries.Add((uint32)key, ret);
			return ret;
		}
	}
	// Gets the wanted type with type checking and seeing if it exists
	template<typename T> const T* GetEnsure(KeyType key) const
	{
		IConfigEntry*const* found = m_entries.Find((uint32)key);
		assert(found);
		const T* targetType = found[0]->As<T>();
		assert(targetType); // Make sure type matches
		return targetType;
	}
};

