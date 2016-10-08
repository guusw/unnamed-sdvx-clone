#include "stdafx.h"
#include "Yaml.hpp"

namespace Yaml
{
	Yaml::MappingIterator& MappingIterator::operator--()
	{
		m_iterator--;
		m_Update();
		return *this;
	}
	Yaml::MappingIterator& MappingIterator::operator++()
	{
		m_iterator++;
		m_Update();
		return *this;
	}
	MappingIterator::operator bool() const
	{
		return m_iterator != m_end;
	}
	bool MappingIterator::operator!=(const MappingIterator& other) const
	{
		return m_iterator != other.m_iterator;
	}
	bool MappingIterator::operator==(const MappingIterator& other) const
	{
		return m_iterator == other.m_iterator;
	}
	MappingIterator::MappingIterator(ObjectMapping& mapping, ItType it, ItType end) : m_mapping(&mapping)
	{
		m_iterator = it;
		m_end = end;
		m_Update();
	}
	void MappingIterator::m_Update()
	{
		if(*this)
		{
			key = *m_iterator;
			value = m_mapping[0][*key];
		}
	}

	Mapping::~Mapping()
	{
		Clear();
	}
	void Mapping::Clear()
	{
		for(auto e : m_items)
			e.second->DecRef();
		for(auto e : m_orderedKeys)
			e->DecRef();
		m_orderedKeys.clear();
		m_items.clear();
#ifdef _DEBUG_YAML_OBJECTS
		m_orderedValues.clear();
#endif
	}
	Yaml::MappingIterator Mapping::begin()
	{
		return { m_items, m_orderedKeys.begin(), m_orderedKeys.end() };
	}
	Yaml::MappingIterator Mapping::end()
	{
		return { m_items, m_orderedKeys.end(), m_orderedKeys.end() };
	}
	void Mapping::Add(Scalar* key, Node* node)
	{
		auto it = m_items.find(*key);
		if(it != m_items.end())
		{
			it->second->DecRef();
			it->second = node;
		}
		else
			m_items.Add(*key, node);
		node->IncRef();
		key->IncRef();
		m_orderedKeys.AddBack(key);
#ifdef _DEBUG_YAML_OBJECTS
		m_orderedValues.Add(node);
#endif
	}

	void Mapping::Add(Scalar key, Node* node)
	{
		Add(new Scalar(key), node);
	}

	uint32 Mapping::GetSize() const
	{
		return (uint32)m_items.size();
	}
	Node* Mapping::Find(const String& key)
	{
		return Find(Scalar(key));
	}
	Yaml::Node* Mapping::Find(Scalar& key)
	{
		Node** ret = m_items.Find(key);
		return ret ? *ret : nullptr;
	}
}