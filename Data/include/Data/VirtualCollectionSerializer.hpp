#pragma once
#include <Shared/Action.hpp>

namespace Data
{
	template<typename TBase>
	struct VirtualType
	{
		Action<TBase*> construct;
		String name;
		size_t hashCode;
	};

	template<typename TBase>
	class VirtualObjectSerializer : public ITypeSerializer
	{
	public:
		template<typename T>
		void RegisterType(const String& name)
		{
			assert(!m_typesByName.Contains(name));
			static_assert(std::is_base_of<TBase, T>::value, "Type must inherit from TBase");

			const type_info& ti = typeid(T);
			auto& typeEntry = m_types.Add(ti.hash_code());
			typeEntry.construct = []()->TBase* { return new T(); };
			typeEntry.hashCode = ti.hash_code();
			typeEntry.name = name;
			m_typesByName.Add(name, typeEntry.hashCode);
		}

		virtual bool Deserialize(Node& data, void* object) override
		{
			String typeTag = data.GetTag();
			if(typeTag.empty())
				return false;
			typeTag = typeTag.substr(1);

			size_t* typeHashCode = m_typesByName.Find(typeTag);
			if(!typeHashCode)
			{
				Logf("No virtual object serializer found for tag %s", Logger::Error, typeTag);
				return false;
			}
			auto& virtualType = m_types[*typeHashCode];

			TBase*& targetObject = *(TBase**)object;
			targetObject = virtualType.construct();

			ITypeSerializer* serializer = Serializer::Find(*typeHashCode);
			if(!serializer)
			{
				Logf("No virtual object serializer found for type %s", Logger::Error, virtualType.name);
				return false;
			}

			return serializer->Deserialize(data, targetObject);
		}
		virtual bool Serialize(const void* object, Node*& data) override
		{
			const type_info& typeInfo = typeid(*(TBase*)object);
			auto foundVirtualType = m_types.Find(typeInfo.hash_code());
			if(!foundVirtualType)
			{
				Logf("Type not registered in virtual object serializer: %s", Logger::Error, typeInfo.name());
				return false;
			}
			auto& virtualType = *foundVirtualType;

			ITypeSerializer* serializer = Serializer::Find(virtualType.hashCode);
			if(!serializer)
			{
				Logf("No virtual object serializer found for type %s", Logger::Error, virtualType.name);
				return false;
			}

			if(!serializer->Serialize(object, data))
				return false;

			// Mark with type ID
			data->SetTag("!" + virtualType.name);

			return true;
		}

	private:
		Map<size_t, VirtualType<TBase>> m_types;
		Map<String, size_t> m_typesByName;
	};

	template<typename TBase>
	class VirtualVectorSerializer : public ITypeSerializer
	{
	public:
		typedef Vector<TBase*> VectorType;

		template<typename T>
		void RegisterType(const String& name)
		{
			assert(!m_typesByName.Contains(name));
			static_assert(std::is_base_of<TBase, T>::value, "Type must inherit from TBase");

			const type_info& ti = typeid(T);
			auto& typeEntry = m_types.Add(ti.hash_code());
			typeEntry.construct = []()->TBase* { return new T(); };
			typeEntry.hashCode = ti.hash_code();
			typeEntry.name = name;
			m_typesByName.Add(name, typeEntry.hashCode);
		}

		virtual bool Deserialize(Node& data, void* object) override
		{
			VectorType& vec = *(VectorType*)object;
			// Should be a sequence
			if(!data.IsSequence())
				return false;
			return Deserialize(data.AsSequence(), vec);
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
		
		// TODO: Add error messages
		bool Deserialize(Sequence& sequence, VectorType& vec)
		{
			// Make sure target is empty
			vec.clear();
			for(auto item : sequence)
			{
				String typeTag = item->GetTag();
				if(typeTag.empty())
					return false;
				typeTag =  typeTag.substr(1);

				size_t* typeHashCode = m_typesByName.Find(typeTag);
				if(!typeHashCode)
					return false; 
				auto& virtualType = m_types[*typeHashCode];

				ITypeSerializer* serializer = Serializer::Find(*typeHashCode);
				if(!serializer)
					return false;

				auto& inner = vec.Add(virtualType.construct());
				if(!serializer->Deserialize(*item, inner))
					return false;
			}

			return true;
		}
		// TODO: Add error messages
		bool Serialize(const VectorType& vec, Sequence& sequence)
		{
			for(auto& item : vec)
			{
				const type_info& typeInfo = typeid(*(TBase*)item);
				auto foundVirtualType = m_types.Find(typeInfo.hash_code());
				if(!foundVirtualType)
					return false;
				auto& virtualType = *foundVirtualType;

				ITypeSerializer* serializer = Serializer::Find(virtualType.hashCode);
				if(!serializer)
					return false;

				Node* elementNode;
				if(!serializer->Serialize(&item, elementNode))
				{
					return false;
				}

				// Mark with type ID
				elementNode->SetTag("!" + virtualType.name);

				sequence.Add(elementNode);
			}
			return true;
		}

	private:
		Map<size_t, VirtualType<TBase>> m_types;
		Map<String, size_t> m_typesByName;
	};
}