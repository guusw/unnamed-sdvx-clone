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

			return Deserialize(sequence, vec);
		}
		virtual bool Serialize(const void* object, Node*& data) override
		{
			VectorType& vec = *(VectorType*)object;

			Sequence* sequence = new Yaml::Sequence();
			data = sequence;
			
			if(!Serialize(vec, *sequence))
			{
				delete sequence;
				return false;
			}
			return true;
		}

		virtual bool Deserialize(Sequence& sequence, VectorType& vec)
		{
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
		virtual bool Serialize(const VectorType& vec, Sequence& sequence)
		{
			ITypeSerializer* innerTypeSerializer = Serializer::Find<T>();
			if(!innerTypeSerializer)
			{
				return false;
			}

			for(auto& item : vec)
			{
				Node* elementNode;
				if(!innerTypeSerializer->Serialize(&item, elementNode))
				{
					return false;
				}
				sequence.Add(elementNode);
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

			return Deserialize(data.AsMapping(), map);
		}
		virtual bool Serialize(const void* object, Node*& data) override
		{
			MapType& map = *(MapType*)object;

			// Create new mapping
			Mapping* mapping = new Yaml::Mapping();
			data = mapping;

			if(!Serialize(map, *mapping))
			{
				delete mapping;
				return false;
			}
			return true;
		}

		virtual bool Deserialize(Mapping& mapping, MapType& map)
		{
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
		virtual bool Serialize(const MapType& map, Mapping& mapping)
		{
			ITypeSerializer* keySerializer = Serializer::Find<K>();
			ITypeSerializer* valueSerializer = Serializer::Find<V>();
			if(!keySerializer || !valueSerializer)
			{
				return false;
			}

			for(auto& item : map)
			{
				Node* keyNode;
				Node* valueNode;
				if(!keySerializer->Serialize(&item.first, keyNode) || !keyNode->IsScalar())
				{
					return false;
				}
				if(!valueSerializer->Serialize(&item.second, valueNode))
				{
					delete keyNode;
					return false;
				}
				mapping.Add((Scalar*)keyNode, valueNode);
			}

			return true;
		}
	};
}