#pragma once
#include "Serializer.hpp"

namespace Data
{
	template<typename T>
	class VectorSerializer : public ITypeSerializer
	{
	public:
		typedef Vector<T> VectorType;

		virtual bool Deserialize(Node& data, void* object) override
		{
			VectorType& vec = *(VectorType*)object;

			// Should be a sequence
			if(!data.IsSequence())
				return false;
			Sequence& sequence = data.AsSequence();

			ITypeSerializer* innerTypeSerializer = Serializer::Find<T>();
			if(!innerTypeSerializer)
				return false;
			
			// Make sure target is empty
			vec.clear();

			for(auto item : sequence)
			{
				T& inner = vec.Add();
				if(!innerTypeSerializer->Deserialize(*item, &inner))
					return false;
			}

			return true;
		}
		virtual bool Serialize(const void* object, Node*& data) override
		{
			VectorType& vec = *(VectorType*)object;

			Sequence* sequence = new Yaml::Sequence();
			data = sequence;

			ITypeSerializer* innerTypeSerializer = Serializer::Find<T>();
			if(!innerTypeSerializer)
			{
				delete sequence;
				return false;
			}

			for(auto& item : vec)
			{
				Node* elementNode;
				if(!innerTypeSerializer->Serialize(&item, elementNode))
				{
					delete sequence;
					return false;
				}
				sequence->Add(elementNode);
			}

			return true;
		}
	};

	template<typename K, typename V>
	class MapSerializer : public ITypeSerializer
	{
	public:
		typedef Map<K, V> MapType;

		virtual bool Deserialize(Node& data, void* object) override
		{
			MapType& map = *(MapType*)object;

			// Should be a mapping
			if(!data.IsMapping())
				return false;
			Mapping& mapping = data.AsMapping();

			ITypeSerializer* keySerializer = Serializer::Find<K>();
			ITypeSerializer* valueSerializer = Serializer::Find<V>();
			if(!keySerializer || !valueSerializer)
				return false;

			// Make sure target is empty
			map.clear();

			for(auto item : mapping)
			{
				K key;
				if(!keySerializer->Deserialize(*item.key, &key))
					return false;
				V& val = map.Add(key);
				if(!valueSerializer->Deserialize(*item.value, &val))
					return false;
			}

			return true;
		}
		virtual bool Serialize(const void* object, Node*& data) override
		{
			MapType& map = *(MapType*)object;

			Mapping* mapping = new Mapping();
			data = mapping;

			ITypeSerializer* keySerializer = Serializer::Find<K>();
			ITypeSerializer* valueSerializer = Serializer::Find<V>();
			if(!keySerializer || !valueSerializer)
			{
				delete mapping;
				return false;
			}

			for(auto& item : map)
			{
				Node* keyNode;
				Node* valueNode;
				if(!keySerializer->Serialize(&item.first, keyNode) || !keyNode->IsScalar())
				{
					delete mapping;
					return false;
				}
				if(!valueSerializer->Serialize(&item.second, valueNode))
				{
					delete mapping;
					delete keyNode;
					return false;
				}
				mapping->Add((Scalar*)keyNode, valueNode);
			}

			return true;
		}
	};
}