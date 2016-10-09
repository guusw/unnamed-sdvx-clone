#pragma once
#include "Serializer.hpp"

namespace Data
{
	/*
	Generic base class that has a common interface for text serializable objects
	Allows adding members in the constructor with AddMember()
	These members, associated with their name with then serialize from and to NodeBase
	*/
	class ClassSerializerBase : public ITypeSerializer
	{
	public:
		virtual ~ClassSerializerBase();
		virtual bool Deserialize(Node& data, void* object);
		virtual bool Serialize(const void* object, Node*& data);

		virtual bool Deserialize(Mapping& map, void* object);
		virtual bool Serialize(const void* object, Mapping& map);

	protected:
		// Adds a data member to serialize
		// must pass the type_info of the type and it's offset in this class
		void AddMember(const String& name, const type_info& type, uint32 offset, Action<uint8*> defaultInitializer = Action<uint8*>());
		void AddMember(const String& name, uint32 offset, ITypeSerializer* customSerializer, Action<uint8*> defaultInitializer = Action<uint8*>());

	protected:
		struct Member
		{
			// Name of the member
			String name;
			// TypeID of the data member
			size_t type;
			// Offset in structure
			uint32 offset;
			// Member ID
			uint32 id;
			// The default initializer
			Action<uint8*> defaultInitializer;
			// Serializer for custom members
			ITypeSerializer* customSerializer;
		};

		Vector<Member*> m_members;
		Map<String, Member*> m_membersByName;
	};

	/*
	Template base class for class serializers, this should be used instead of ClassSerializerBase
	unless you need to specify custom constructors and destructors
	*/
	template<typename T>
	class ClassSerializer : public ClassSerializerBase
	{
	public:
		typedef ClassSerializer<T> Base;
		typedef T SerializedType;

		virtual bool Deserialize(Mapping& map, void* object) override
		{
			if(!ClassSerializerBase::Deserialize(map, object))
				return false;
			return DeserializeUserProperties(map, *(T*)object);
		}
		virtual bool Serialize(const void* object, Mapping& map) override
		{
			if(!ClassSerializerBase::Serialize(object, map))
				return false;
			return SerializeUserProperties(*(const T*)object, map);
		}

		// Class specific override
		virtual bool DeserializeUserProperties(Mapping& map, T& object)
		{
			return true;
		}
		virtual bool SerializeUserProperties(const T& object, Mapping& map)
		{
			return true;
		}
	};

	template<typename T, typename B>
	class BaseClassSerializer : public ClassSerializer<T>
	{
	public:
		typedef B BaseClassType;
		virtual bool Deserialize(Node& data, void* object) override
		{
			ITypeSerializer* baseSerializer = Serializer::Find<B>();
			if(!baseSerializer)
				return false;
			if(!baseSerializer->Deserialize(data, object))
				return false;
			return ClassSerializer<T>::Deserialize(data.AsMapping(), object);
		}
		virtual bool Serialize(const void* object, Node*& data) override
		{
			ITypeSerializer* baseSerializer = Serializer::Find<B>();
			if(!baseSerializer)
				return false;
			if(!baseSerializer->Serialize(object, data))
				return false;
			return ClassSerializer<T>::Serialize(object, data->AsMapping());
		}
	};

	struct RegisterSerializerHelper
	{
		RegisterSerializerHelper(const type_info& type, ITypeSerializer* serializer)
		{
			Serializer::Register(type, serializer);
		}
	};
}

#define RegisterSerializer(__serializerType)\
static Data::RegisterSerializerHelper* CONCAT(__rsh, __LINE__) = \
	new Data::RegisterSerializerHelper(typeid(__serializerType::SerializedType), new __serializerType());
#define RegisterClassMember(__member) AddMember(STRINGIFY(__member), typeid(CONCAT(SerializedType,CONCAT(::, __member))), offsetof(SerializedType, __member));
#define RegisterClassMemberCustomSerializer(__name, __member, __serializer)  AddMember(__name, offsetof(SerializedType, __member), __serializer);
#define RegisterClassMemberNamed(__name, __member) AddMember(__name, typeid(CONCAT(SerializedType,CONCAT(::, __member))), offsetof(SerializedType, __member));