#include "stdafx.h"
#include "PropertyHierarchy.hpp"

namespace PropertyHierarchy
{

	NodeBase::NodeBase(const String& name /*= String()*/)
	{
		m_name = name;
	}
	const String& NodeBase::GetName() const
	{
		return m_name;
	}
	void NodeBase::SetName(const String& name)
	{
		m_name = name;
	}
	void NodeBase::SetParent(NodeBase* parent)
	{
		m_parent = parent;
	}
	const NodeBase* NodeBase::GetParent() const
	{
		return m_parent;
	}
	
	Object::~Object()
	{
		Clear();
	}
	void Object::Set(const String& name, const NodeBase& right)
	{
		auto existing = m_children.find(name);
		NodeBase* clone = right.Clone();
		clone->SetParent(this);
		if(existing != m_children.end())
		{
			delete existing->second;
			existing->second = clone;
		}
		else
		{
			m_children.Add(name, clone);
		}
	}
	const NodeBase* Object::Get(const String& key) const
	{
		auto found = m_children.Find(key);
		return found ? *found : nullptr;
	}
	NodeBase* Object::Get(const String& key)
	{
		auto found = m_children.Find(key);
		return found ? *found : nullptr;
	}
	void Object::Clear()
	{
		for(auto c : m_children)
			delete c.second;
		m_children.clear();
	}
	NodeBase* Object::Clone() const
	{
		Object* ret = new Object(m_name);
		for(auto& c : m_children)
		{
			ret->Set(c.first, *c.second);
		}
		return ret;
	}

	List::~List()
	{
		Clear();
	}
	void List::Add(const NodeBase& node)
	{
		NodeBase* clone = node.Clone();
		clone->SetParent(this);
		m_children.Add(clone);
	}
	void List::Remove(uint32 index)
	{
		assert(index < m_children.size());
		delete m_children[index];
		m_children.erase(m_children.begin() + index);
	}
	void List::Clear()
	{
		for(auto& c : m_children)
			delete c;
		m_children.clear();
	}
	uint32 List::GetSize()
	{
		return (uint32)m_children.size();
	}
	const NodeBase* List::Get(uint32 index) const
	{
		assert(index < m_children.size());
		return m_children[index];
	}
	NodeBase* List::Get(uint32 index)
	{
		assert(index < m_children.size());
		return m_children[index];
	}

	NodeBase* List::Clone() const
	{
		List* ret = new List(m_name);
		for(auto& c : m_children)
			ret->Add(*c);
		return ret;
	}
}