#include "stdafx.h"
#include "ClassSerializer.hpp"

namespace Data
{
	using namespace Yaml;

	void RegisterDefaultSerializers();

	class Serializer_Impl
	{
	public:
		Map<size_t, ITypeSerializer*> serializers;
		Map<String, size_t> typesByName;

		Serializer_Impl()
		{
		}
		static Serializer_Impl& Get()
		{
			static Serializer_Impl instance;
			static bool initialized = false;
			if(!initialized)
			{
				initialized = true;
				RegisterDefaultSerializers();
			}
			return instance;
		}
	};

	ITypeSerializer* Serializer::Find(size_t type)
	{
		Serializer_Impl& instance = Serializer_Impl::Get();
		ITypeSerializer** serializer = instance.serializers.Find(type);
		if(serializer)
			return *serializer;
		return nullptr;
	}
	void Serializer::Register(const type_info& type, ITypeSerializer* serializerInstance)
	{
		Serializer_Impl& instance = Serializer_Impl::Get();
		assert(!instance.serializers.Contains(type.hash_code()));

		instance.serializers.Add(type.hash_code(), serializerInstance);
		instance.typesByName.Add(type.name(), type.hash_code());
	}
}