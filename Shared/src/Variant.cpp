#include "stdafx.h"
#include "Variant.hpp"
#include "BinaryStream.hpp"

float Variant::ToFloat() const
{
	assert(m_variant);
	return m_variant->ToFloat();
}
String Variant::ToString() const
{
	assert(m_variant);
	return m_variant->ToString();
}
VariantType Variant::GetType() const
{
	return m_variant ? m_variant->GetType() : VariantType::Null;
}
Variant& Variant::operator=(Variant&& other)
{
	m_Clear();
	m_variant = other.m_variant;
	other.m_variant = nullptr;
	return *this;
}
Variant& Variant::operator=(const Variant& other)
{
	m_Clear();
	if(other.m_variant)
		m_variant = other.m_variant->Clone();
	return *this;
}
void Variant::m_Clear()
{
	if(m_variant)
		delete m_variant;
	m_variant = nullptr;
}

Variant Variant::Create(const char* val)
{
	return Variant::Create(String(val));
}

bool Variant::StaticSerialize(class BinaryStream& stream, Variant*& object)
{
	uint8 type = stream.IsReading() ? 0 : (uint8)object->GetType();
	stream << type;
	switch(type)
	{
	default:
	case VariantType::Null:
		if(stream.IsReading())
			object = new Variant();
		break;
	case VariantType::Float:
	{
		float v = 0.0f;
		if(!stream.IsReading() && object->m_variant)
			v = object->ToFloat();
		stream << v;
		if(stream.IsReading())
			object = new Variant(Variant::Create(v));
	}
		break;
	case VariantType::Int:
	{
		int32 v = 0;
		if(!stream.IsReading() && object->m_variant)
			v = object->ToInt();
		stream << v;
		if(stream.IsReading())
			object = new Variant(Variant::Create(v));
	}
		break;
	case VariantType::String:
	{
		String v = String();
		if(!stream.IsReading() && object->m_variant)
			v = object->ToString();
		stream << v;
		if(stream.IsReading())
			object = new Variant(Variant::Create(v));
	}
		break;
	}

	return true;
}

Variant::Variant(Variant&& other)
{
	m_variant = other.m_variant;
	other.m_variant = nullptr;
}
Variant::Variant(const Variant& other)
{
	if(other.m_variant)
		m_variant = other.m_variant->Clone();
}
Variant::Variant()
{

}
Variant::~Variant()
{
	m_Clear();
}
int32 Variant::ToInt() const
{
	assert(m_variant);
	return m_variant->ToInt();
}

// Int variant type
TVariant<int32>::TVariant(int32 data) : m_data(data)
{
}
int32 TVariant<int32>::ToInt() const
{
	return m_data;
}
float TVariant<int32>::ToFloat() const
{
	return (float)m_data;
}
String TVariant<int32>::ToString() const
{
	return Utility::Sprintf("%d", m_data);
}
IVariant* TVariant<int32>::Clone() const
{
	return new TVariant<int32>(m_data);
}
VariantType TVariant<int32>::GetType() const
{
	return VariantType::Int;
}

// Float variant type
TVariant<float>::TVariant(float data) : m_data(data)
{
}
int32 TVariant<float>::ToInt() const
{
	return (int32)m_data;
}
float TVariant<float>::ToFloat() const
{
	return m_data;
}
String TVariant<float>::ToString() const
{
	return Utility::Sprintf("%f", m_data);
}
IVariant* TVariant<float>::Clone() const
{
	return new TVariant<float>(m_data);
}
VariantType TVariant<float>::GetType() const
{
	return VariantType::Float;
}

// String variant type
TVariant<String>::TVariant(String data) : m_data(data)
{
}
int32 TVariant<String>::ToInt() const
{
	return atoi(*m_data);
}
float TVariant<String>::ToFloat() const
{
	return (float)atof(*m_data);
}
String TVariant<String>::ToString() const
{
	return m_data;
}
IVariant* TVariant<String>::Clone() const
{
	return new TVariant<String>(m_data);
}
VariantType TVariant<String>::GetType() const
{
	return VariantType::String;
}
