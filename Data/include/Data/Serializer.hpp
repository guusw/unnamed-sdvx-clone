#pragma once
#include "Data/Yaml.hpp"
#include "Shared/Action.hpp"
#include "Shared/Macro.hpp"
#include <typeinfo>

namespace Data
{
	using namespace Yaml;
	class ITypeSerializer
	{
	public:
		virtual ~ITypeSerializer() = default;
		// Convert an data node to an object
		virtual bool Deserialize(Node& data, void* object) = 0;
		// Creates a data node from an object
		virtual bool Serialize(const void* object, Node*& data) = 0;
	};

	/*
		Serializes objects to and from a textual representatation
	*/
	class Serializer
	{
	public:
		template<typename T>
		static ITypeSerializer* Find() { return Find(typeid(T).hash_code()); }
		static ITypeSerializer* Find(size_t type);

		template<typename T>
		static void Register(ITypeSerializer* serializerInstance) { Register(typeid(T), serializerInstance); }
		static void Register(const type_info& type, ITypeSerializer* serializerInstance);
	};
}