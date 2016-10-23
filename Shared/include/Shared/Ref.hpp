#pragma once
#include <assert.h>
#include <type_traits>

template<typename T>
class RefCounted;

/*
	Basic shared pointer class
	the object should be constructed once explicitly with a pointer to the object to manage
	When there are no more references to this object, the underlying object gets deleted
*/
template<typename T>
class Ref
{
protected:
	T* m_data;
	int32* m_refCount;
	void m_Dec()
	{
		if(m_refCount)
		{
			assert(m_refCount[0] != 0);
			if(m_refCount[0] >= 0)
			{
				m_refCount[0]--;
				if(m_refCount[0] == 0)
				{
					delete m_data;
					delete m_refCount;
					m_refCount = nullptr;
#if _DEBUG
					m_data = nullptr;
#endif
				}
			}
			else
			{
				m_refCount[0]++;
				if(m_refCount[0] == 0)
				{
					delete m_refCount;
					m_refCount = nullptr;
#if _DEBUG
					m_data = nullptr;
#endif
				}
			}
		}
	}
	void m_Inc()
	{
		if(m_refCount)
		{
			if(m_refCount[0] >= 0)
			{
				m_refCount[0]++;
			}
			else
			{
				m_refCount[0]--;
			}
		}
	}
	void m_AssignCounter();
	int32* m_CreateNewCounter();

public:
	// Explicit ref count initializer
	explicit Ref(T* data, int32_t* refCount)
		: m_data(data), m_refCount(refCount)
	{
		m_Inc();
	}

	// Null/Default constructor
	Ref()
	{
		m_data = nullptr;
		m_refCount = nullptr;
	}

	explicit Ref(T* data)
	{
		m_data = data;
		m_refCount = m_CreateNewCounter();
		m_refCount[0]++;
		m_AssignCounter();
	}

	~Ref()
	{
		m_Dec();
	}
	Ref(const Ref& other)
	{
		m_data = other.m_data;
		m_refCount = other.m_refCount;
		m_Inc();
	}
	Ref(Ref&& other)
	{
		m_data = other.m_data;
		m_refCount = other.m_refCount;
		other.m_refCount = nullptr;
	}
	Ref& operator=(const Ref& other)
	{
		m_Dec();
		m_data = other.m_data;
		m_refCount = other.m_refCount;
		m_Inc();
		return *this;
	}
	Ref& operator=(Ref&& other)
	{
		m_Dec();
		m_data = other.m_data;
		m_refCount = other.m_refCount;
		other.m_refCount = nullptr;
		return *this;
	}
	T& operator*()
	{
		assert(IsValid());
		return *m_data;
	}
	const T& operator*() const
	{
		assert(IsValid());
		return *m_data;
	}
	T* operator->()
	{
		assert(IsValid());
		return m_data;
	}
	const T* operator->() const
	{
		assert(IsValid());
		return m_data;
	}

	template<typename Target>
	Target* Cast()
	{
		if(!m_refCount)
			return nullptr;
		return dynamic_cast<Target*>(m_data);
	}
	template<typename Target>
	const Target* Cast() const
	{
		if(!m_refCount)
			return nullptr;
		return dynamic_cast<Target*>(m_data);
	}
	// Casts the reference to a different type while not breaking the reference counting
	template<typename Target>
	Ref<Target> As()
	{
		Target* castData = dynamic_cast<Target*>(m_data);
		if(!m_refCount || !castData)
			return Ref<Target>();
		return Ref<Target>(castData, m_refCount);
	}

	bool operator<(const Ref& other) const
	{
		return m_refCount < other.m_refCount;
	}
	bool operator==(const Ref& other) const
	{
		return m_refCount == other.m_refCount;
	}
	bool operator!=(const Ref& other) const
	{
		return m_refCount != other.m_refCount;
	}
	bool operator==(const T* other) const
	{
		return m_refCount && m_data == other;
	}
	bool operator!=(const T* other) const
	{
		return m_refCount && m_data != other;
	}

	void Destroy()
	{
		assert(IsValid());
		assert(m_refCount[0] > 0);
		m_refCount[0] = -m_refCount[0] + 1;
		m_refCount = nullptr;
		delete m_data;
	}
	void Release()
	{
		m_Dec();
		m_refCount = nullptr;
	}

	bool IsValid() const { return m_refCount != nullptr && m_refCount[0] > 0; }
	operator bool() const { return IsValid(); }

	int32_t GetRefCount() const
	{
		if(m_refCount && m_refCount[0] > 0)
			return m_refCount[0];
		else
			return 0;
	}

	T* GetData()
	{
		assert(IsValid());
		return m_data;
	}
	const T* GetData() const
	{
		assert(IsValid());
		return m_data;
	}
};

namespace Utility
{
	template<typename T>
	Ref<T> MakeRef(T* obj)
	{
		return Ref<T>(obj);
	}
}

/*
	Base class for objects that allows them to create a shared pointer from themselves
	WARNING: Only use on objects allocated with "new"
	Repeatedly calling MakeShared will return the same shared pointer
*/
class IRefCounted
{
protected:
	int32* m_refCount = (int32*)0;
public:
#if _DEBUG
	virtual ~IRefCounted()
	{
		// Should never happen, object will always 
		assert(!m_refCount || m_refCount[0] <= 0);
	}
#endif
	int32 GetRefCount() const
	{
		return (m_refCount) ? m_refCount[0] : 0;
	}
	// Internal use, assigns the reference counter when constructing a Ref object without calling RefCounted::MakeShared
	void _AssignRefCounter(int32* counter)
	{
		assert(m_refCount == nullptr || m_refCount == counter);
		m_refCount = counter;
	}
	int32* _GetRefCounter()
	{
		if(!m_refCount)
			m_refCount = new int32(0);
		return m_refCount;
	}
};

// Same as above but typed version
template<typename T>
class RefCounted : public IRefCounted
{
public:
	// Can be used for objects allocated with new to get a reference counted handle to this object
	Ref<T> MakeShared()
	{
		return Ref<T>((T*)this, _GetRefCounter());
	}
	operator Ref<T>()
	{
		return MakeShared();
	}
};

// Possibly assign reference counter in RefCounted object
template<typename T, bool>
struct RefCounterHelper
{
	static void Assign(T* obj, int32* counter)
	{
	}
	static int32* CreateCounter(T* obj)
	{
		return new int32(0);
	}
};

template<typename T>
struct RefCounterHelper<T, true>
{
	static void Assign(T* obj, int32* counter)
	{
		obj->_AssignRefCounter(counter);
	}
	static int32* CreateCounter(T* obj)
	{
		return obj->_GetRefCounter();
	}
};

template<typename T>
void Ref<T>::m_AssignCounter()
{
	assert(m_data);
	RefCounterHelper<T, std::is_base_of<IRefCounted, T>::value>::Assign((T*)m_data, m_refCount);
}
template<typename T>
int32* Ref<T>::m_CreateNewCounter()
{
	assert(m_data);
	return RefCounterHelper<T, std::is_base_of<IRefCounted, T>::value>::CreateCounter((T*)m_data);
}
