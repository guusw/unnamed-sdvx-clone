#pragma once

#if defined(_MSC_VER) && defined(_DEBUG)
// Used to allow debug visualizers for these objects to work
#define _DEBUG_YAML_OBJECTS
#endif

namespace Yaml
{
	class Node
	{
	public:
		virtual ~Node() = default;

		bool IsScalar() const;
		class Scalar& AsScalar();

		bool IsMapping() const;
		class Mapping& AsMapping();

		bool IsSequence() const;
		class Sequence& AsSequence();

		// Used to set the application specific type tag 
		void SetTag(const String& tag);
		// Used to get the application specific type tag
		const String& GetTag() const;

		void IncRef();
		void DecRef();
	private:
		int m_ref = 0;
		String m_tag;
	};

	class Scalar : public Node
	{
	public:
		// Null scalar constructor
		Scalar();
		Scalar(const char* stringInitializer);
		Scalar(const String& stringScalar);
		Scalar(float floatScalar);
		Scalar(int32 intScalar);
		explicit Scalar(bool booleanScalar);

		bool operator ==(const Scalar& right) const;
		bool operator <(const Scalar& right) const;
		bool operator >(const Scalar& right) const;
		bool operator ==(const String& right) const;
		bool operator <(const String& right) const;
		bool operator >(const String& right) const;

		bool IsNull() const;
		bool IsNumber() const;
		bool IsBoolean() const;

		int32 ToInt() const;
		float ToFloat() const;
		String ToString() const;
		bool ToBoolean() const;

	private:
		// Tries to detect the type of the input string
		void m_DetectType();

		enum Type
		{
			T_Null = 0,
			T_String,
			T_Number,
			T_Boolean,
		};
		String m_data;
		Type m_detectedType;
	};
}

#include "YamlMapping.hpp"
#include "YamlSequence.hpp"