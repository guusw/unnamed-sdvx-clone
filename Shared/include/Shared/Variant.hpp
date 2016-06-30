#pragma once
#include "Shared/String.hpp"

enum class VariantType
{
	Null,
	Int,
	Float,
	String,
	Compound,
};

class IVariant
{
public:
	virtual ~IVariant() = default;
	virtual int32 ToInt() const = 0;
	virtual float ToFloat() const = 0;
	virtual String ToString() const = 0;
	virtual IVariant* Clone() const = 0;
	virtual VariantType GetType() const = 0;
};

template<typename T>
class TVariant
{
};

class Variant
{
private:
	void m_Clear();

	IVariant* m_variant = nullptr;

public:
	template<typename T> static Variant Create(const T& val);
	static Variant Create(const char* val);
	// Serialization helper for variants
	static bool StaticSerialize(class BinaryStream& stream, Variant*& object);
	Variant();
	~Variant();
	Variant(const Variant& other);
	Variant(Variant&& other);
	Variant& operator=(const Variant& other);
	Variant& operator=(Variant&& other);
	int32 ToInt() const;
	float ToFloat() const;
	String ToString() const;
	VariantType GetType() const;
};

template<typename T>
Variant Variant::Create(const T& val)
{
	Variant r;
	r.m_variant = new TVariant<T>(val);
	return std::move(r);
}

// Int variant type
template<> class TVariant<int32> : public IVariant
{
	int32 m_data;
public:
	TVariant(int32 data = 0);
	virtual int32 ToInt() const;
	virtual float ToFloat() const;
	virtual String ToString() const;
	virtual IVariant* Clone() const;
	virtual VariantType GetType() const;
};


// Float variant type
template<> class TVariant<float> : public IVariant
{
	float m_data;
public:
	TVariant(float data = 0);
	virtual int32 ToInt() const;
	virtual float ToFloat() const;
	virtual String ToString() const;
	virtual IVariant* Clone() const;
	virtual VariantType GetType() const;
};

// String variant type
template<> class TVariant<String> : public IVariant
{
	String m_data;
public:
	TVariant(String data = String());
	virtual int32 ToInt() const;
	virtual float ToFloat() const;
	virtual String ToString() const;
	virtual IVariant* Clone() const;
	virtual VariantType GetType() const;
};