/*
	Header file for the property hierarchies
*/
#pragma once
#include "String.hpp"
#include "Vector.hpp"
#include "Map.hpp"
#include "Unique.hpp"

namespace PropertyHierarchy
{
	class NodeBase : public Unique
	{
	public:
		NodeBase(const String& name = String());
		virtual ~NodeBase() = default;
		const String& GetName() const;
		void SetName(const String& name);

		virtual NodeBase* Clone() const = 0;
		void SetParent(NodeBase* parent);
		const NodeBase* GetParent() const;

	protected:
		String m_name;
		NodeBase* m_parent;
	};

	/* 
		An object, which contains members assigned to keys
		works much the same as a javascript object
	*/
	class Object : public NodeBase
	{
	public:
		using NodeBase::NodeBase;
		~Object();

		void Set(const String& key, const NodeBase& right);
		const NodeBase* Get(const String& key) const;
		NodeBase* Get(const String& key);
		void Clear();

		virtual NodeBase* Clone() const;

	protected:
		Map<String, NodeBase*> m_children;
	};

	/*
		A list objects,
		contains child objects accessible by index
	*/
	class List : public NodeBase
	{
	public:
		using NodeBase::NodeBase;
		~List();
		void Add(const NodeBase& node);
		void Remove(uint32 index);
		const NodeBase* Get(uint32 index) const; 
		NodeBase* Get(uint32 key);
		void Clear();
		uint32 GetSize();

		virtual NodeBase* Clone() const;

	protected:
		Vector<NodeBase*> m_children;
	};

}