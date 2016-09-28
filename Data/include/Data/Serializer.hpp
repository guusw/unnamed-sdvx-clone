#pragma once
#include "Data/PropertyHierarchy.hpp"
#include <typeinfo>

namespace Data
{
	/*
		Serializes objects to and from a textual representatation
	*/
	class Serializer
	{
	public:
	};

	/*
		Generic base class that has a common interface for text serializable objects
	*/
	class TypeSerializerBase
	{
	public:
		virtual ~TypeSerializerBase() = default;
		// Store the text representatation of object in out
		virtual bool ToString(const NodeBase& object, String& out) = 0;
		// Store the parsed object allocated with 'new' in out
		virtual bool FromString(const String& input, NodeBase* out) = 0;

		void AddMember(type_info type, void* handle);
	};

	/*
		Templated base class for all types that want to implement text serialization
	*/
	template<typename T>
	class TypeSerializer : public TypeSerializerBase
	{
	public:
		TypeSerializer();

		void AddMember();
	};
}