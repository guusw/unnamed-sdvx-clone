#include "stdafx.h"
#include "Yaml.hpp"

namespace Yaml
{
	bool Node::IsScalar() const
	{
		return dynamic_cast<const Scalar*>(this) != nullptr;
	}
	Yaml::Scalar& Node::AsScalar()
	{
		return dynamic_cast<Scalar&>(*this);
	}
	bool Node::IsMapping() const
	{
		return dynamic_cast<const Mapping*>(this) != nullptr;
	}
	class Mapping& Node::AsMapping()
	{
		return dynamic_cast<Mapping&>(*this);
	}
	bool Node::IsSequence() const
	{
		return dynamic_cast<const Sequence*>(this) != nullptr;
	}
	class Sequence& Node::AsSequence()
	{
		return dynamic_cast<Sequence&>(*this);
	}
	void Node::SetTag(const String& tag)
	{
		m_tag = tag;
	}
	const String& Node::GetTag() const
	{
		return m_tag;
	}
	void Node::IncRef()
	{
		m_ref++;
	}
	void Node::DecRef()
	{
		if(--m_ref == 0)
			delete this;
	}

	Scalar::Scalar()
	{
		m_detectedType = T_Null;
	}
	Scalar::Scalar(const char* stringInitializer)
	{
		m_data = stringInitializer;
		m_DetectType();
	}
	Scalar::Scalar(const String& input)
	{
		m_data = input;
		m_DetectType();
	}
	Scalar::Scalar(float floatScalar)
	{
		m_data = Utility::Sprintf("%f", floatScalar);
		m_detectedType = T_Number;
	}
	Scalar::Scalar(int32 intScalar)
	{
		m_data = Utility::Sprintf("%d", intScalar);
		m_detectedType = T_Number;
	}
	Scalar::Scalar(bool booleanScalar)
	{
		m_data = booleanScalar ? "true" : "false";
		m_detectedType = T_Boolean;
	}
	bool Scalar::operator==(const Scalar& right) const
	{
		return m_data == right.m_data;
	}
	bool Scalar::operator<(const Scalar& right) const
	{
		return m_data < right.m_data;
	}
	bool Scalar::operator>(const Scalar& right) const
	{
		return m_data > right.m_data;
	}

	bool Scalar::operator==(const String& right) const
	{
		return m_data == right;
	}
	bool Scalar::operator<(const String& right) const
	{
		return m_data < right;
	}
	bool Scalar::operator>(const String& right) const
	{
		return m_data > right;
	}

	String Scalar::ToString() const
	{
		return m_data;
	}
	bool Scalar::ToBoolean() const
	{
		if(m_detectedType == T_Boolean)
			return m_data == "true";
		return ToInt() != 0;
	}
	float Scalar::ToFloat() const
	{
		float out;
		sscanf(*m_data, "%f", &out);
		return out;
	}
	int32 Scalar::ToInt() const
	{
		return (int32)strtol(*m_data, nullptr, 0);
	}
	void Scalar::m_DetectType()
	{
		static int32 dummyInteger = 0;
		if(m_data.empty() || m_data == "null")
			m_detectedType = T_Null;
		else if(sscanf(*m_data, "%d", &dummyInteger) > 0)
			m_detectedType = T_Number;
		else if(m_data == "true" || m_data == "false")
			m_detectedType = T_Boolean;
		else
			m_detectedType = T_String;
	}
	bool Scalar::IsNull() const
	{
		return m_detectedType == T_Null;
	}
	bool Scalar::IsBoolean() const
	{
		return m_detectedType == T_Boolean;
	}
	bool Scalar::IsNumber() const
	{
		return m_detectedType == T_Number;
	}
}