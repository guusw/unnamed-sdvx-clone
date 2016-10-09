#pragma once

namespace Yaml
{
	class SequenceIterator
	{
	public:
		Node* value;

		SequenceIterator& operator--();
		SequenceIterator& operator++();
		operator bool() const;
		bool operator!=(const SequenceIterator& other) const;
		bool operator==(const SequenceIterator& other) const;
		Node* operator*() { return value; };

	private:
		friend class Sequence;
		typedef Vector<Node*>::iterator ItType;
		SequenceIterator(ItType it, ItType end);
		void m_Update();

		ItType m_iterator;
		ItType m_end;
	};

	// A Sequence/List object 
	class Sequence : public Node, Unique
	{
	public:
		~Sequence();
		void Clear();

		SequenceIterator begin();
		SequenceIterator end();

		Node& operator[](size_t index);

		uint32 GetSize() const;
		Node& Get(uint32 index);
		void Remove(uint32 index);
		void Add(Scalar&& scalar);
		void Add(Node* object);

	private:
		Vector<Node*> m_items;
	};
}
