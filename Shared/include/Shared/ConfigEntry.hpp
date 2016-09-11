#pragma once
#include "Shared/String.hpp"

class IConfigEntry
{
public:
	virtual ~IConfigEntry() = default;
	// Converts entry to string value
	virtual String ToString() const = 0;
	// Sets the current entry from a string value
	virtual void FromString(const String& str) = 0;

	template<typename T> T* As() { return dynamic_cast<T*>(this); }
};

class IntConfigEntry : public IConfigEntry
{
public:
	int32 data;
public:
	virtual String ToString() const override;
	virtual void FromString(const String& str) override;
};

class BoolConfigEntry : public IConfigEntry
{
public:
	bool data;
public:
	virtual String ToString() const override;
	virtual void FromString(const String& str) override;
}; 

class FloatConfigEntry : public IConfigEntry
{
public:
	float data;
public:
	virtual String ToString() const override;
	virtual void FromString(const String& str) override;
};

class StringConfigEntry : public IConfigEntry
{
public:
	String data;
public:
	virtual String ToString() const override;
	virtual void FromString(const String& str) override;
};

template<typename EnumClass>
class EnumConfigEntry : public IConfigEntry
{
public:
	typename EnumClass::EnumType data;
public:
	virtual String ToString() const override
	{
		return EnumClass::ToString(data);
	}
	virtual void FromString(const String& str) override
	{
		data = EnumClass::FromString(str);
	}
};