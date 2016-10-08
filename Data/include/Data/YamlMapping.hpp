#pragma once
#include <Shared/UnorderedMap.hpp>

namespace Yaml
{
	struct ScalarHash
	{
		size_t operator()(const Scalar& scalar) const
		{
			static StringHash sh;
			return sh(scalar.ToString());
		}
	};

	typedef UnorderedMap<Scalar, Node*, ScalarHash> ObjectMapping;
	typedef List<Scalar*> OrderedKeys;

	class MappingIterator
	{
	public:
		struct Pair
		{
			Scalar* key;
			Node* value;
		};
		Scalar* key;
		Node* value;

		MappingIterator& operator--();
		MappingIterator& operator++();
		operator bool() const;
		bool operator!=(const MappingIterator& other) const;
		bool operator==(const MappingIterator& other) const;
		Pair operator*() { return{ key, value }; };

	private:
		friend class Mapping;
		typedef OrderedKeys::iterator ItType;
		MappingIterator(ObjectMapping& mapping, ItType it, ItType end);
		void m_Update();

		ObjectMapping* m_mapping;
		ItType m_iterator;
		ItType m_end;
	};

	// A map object
	class Mapping : public Node, Unique
	{
	public:
		~Mapping();
		void Clear();

		MappingIterator begin();
		MappingIterator end();

		void Add(Scalar key, Node* node);
		void Add(Scalar* key, Node* node);
		uint32 GetSize() const;

		// Returns either the node with given key or null
		Node* Find(const String& key);
		Node* Find(Scalar& key);

	private:
		OrderedKeys m_orderedKeys;
		ObjectMapping m_items;

#ifdef _DEBUG_YAML_OBJECTS
		Vector<Node*> m_orderedValues;
#endif
	};
}
