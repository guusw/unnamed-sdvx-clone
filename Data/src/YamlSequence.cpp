#include "stdafx.h"
#include "Yaml.hpp"

namespace Yaml
{
	Yaml::SequenceIterator& SequenceIterator::operator--()
	{
		m_iterator--;
		m_Update();
		return *this;
	}
	Yaml::SequenceIterator& SequenceIterator::operator++()
	{
		m_iterator++;
		m_Update();
		return *this;
	}
	SequenceIterator::operator bool() const
	{
		return m_iterator != m_end;
	}
	bool SequenceIterator::operator!=(const SequenceIterator& other) const
	{
		return m_iterator != other.m_iterator;
	}
	bool SequenceIterator::operator==(const SequenceIterator& other) const
	{
		return m_iterator == other.m_iterator;
	}
	SequenceIterator::SequenceIterator(ItType it, ItType end)
	{
		m_iterator = it;
		m_end = end;
		m_Update();
	}
	void SequenceIterator::m_Update()
	{
		if(*this)
		{
			value = *m_iterator;
		}
	}

	Sequence::~Sequence()
	{
		Clear();
	}
	void Sequence::Clear()
	{
		for(auto e : m_items)
			e->DecRef();
		m_items.clear();
	}
	Yaml::SequenceIterator Sequence::begin()
	{
		return { m_items.begin(), m_items.end() };
	}
	Yaml::SequenceIterator Sequence::end()
	{
		return { m_items.end(), m_items.end() };
	}
	Yaml::Node& Sequence::operator[](size_t index)
	{
		return Get((uint32)index);
	}
	Yaml::Node& Sequence::Get(uint32 index)
	{
		assert(index < m_items.size());
		return *m_items[index];
	}
	void Sequence::Remove(uint32 index)
	{
		assert(index < m_items.size());
		m_items.erase(m_items.begin() + index);
	}
	void Sequence::Add(Scalar&& scalar)
	{
		Scalar* newScalar = new Scalar(std::move(scalar));
		newScalar->IncRef();
		m_items.Add(newScalar);
	}
	void Sequence::Add(Node* object)
	{
		object->IncRef();
		m_items.push_back(object);
	}
	uint32 Sequence::GetSize() const
	{
		return (uint32)m_items.size();
	}
}