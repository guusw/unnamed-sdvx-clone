#include "stdafx.h"
#include "ClassSerializer.hpp"

namespace Data
{
	ClassSerializerBase::~ClassSerializerBase()
	{
		for(auto member : m_members)
		{
			delete member;
		}
	}
	bool ClassSerializerBase::Deserialize(Node& data, void* object)
	{
		if(!data.IsMapping())
			return false; // TODO: log a useful error message
		return Deserialize(data.AsMapping(), object);
	}
	bool ClassSerializerBase::Serialize(const void* object, Node*& data)
	{
		data = new Mapping();
		return Serialize(object, data->AsMapping());
	}
	bool ClassSerializerBase::Serialize(const void* object, Mapping& map)
	{
		uint8* objectData = (uint8*)object;
		for(auto member : m_members)
		{
			uint8* memberData = objectData + member->offset;

			ITypeSerializer* memberSerializer = member->customSerializer ?
				member->customSerializer : Serializer::Find(member->type);
			if(!memberSerializer)
			{
				return false; // TODO: log a useful error message
			}

			Node* childNode;
			memberSerializer->Serialize((void*)memberData, childNode);

			map.Add(new Scalar(member->name), childNode);
		}
		return true;
	}

	bool ClassSerializerBase::Deserialize(Mapping& map, void* object)
	{
		uint8* objectData = (uint8*)(object);
		for(auto member : m_members)
		{
			Node* child = map.Find(member->name);
			if(child)
			{
				uint8* memberData = objectData + member->offset;

				ITypeSerializer* memberSerializer = member->customSerializer ? 
					member->customSerializer : Serializer::Find(member->type);
				if(!memberSerializer)
				{
					return false; // TODO: log a useful error message
				}

				memberSerializer->Deserialize(*child, (void*&)memberData);
			}
		}
		return true;
	}

	void ClassSerializerBase::AddMember(const String& name, const type_info& type, uint32 offset, Action<uint8*> defaultInitializer)
	{
		// Check if name is not already mapped 
		assert(!m_membersByName.Contains(name));

		Member* newMember = new Member();
		newMember->type = type.hash_code();
		newMember->id = (uint32)m_members.size();
		newMember->offset = offset;
		newMember->name = name;
		newMember->defaultInitializer = std::move(defaultInitializer);

		// Register
		m_members.Add(newMember);
		m_membersByName.Add(name, newMember);
	}
	void ClassSerializerBase::AddMember(const String& name, uint32 offset, ITypeSerializer* customSerializer, Action<uint8*> defaultInitializer)
	{
		// Check if name is not already mapped 
		assert(!m_membersByName.Contains(name));

		Member* newMember = new Member();
		newMember->id = (uint32)m_members.size();
		newMember->offset = offset;
		newMember->name = name;
		newMember->customSerializer = customSerializer;
		newMember->defaultInitializer = std::move(defaultInitializer);

		// Register
		m_members.Add(newMember);
		m_membersByName.Add(name, newMember);
	}
}