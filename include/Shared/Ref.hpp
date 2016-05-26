#pragma once
#include <assert.h>

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
	int32_t* m_refCount;
	void Dec()
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
	void Inc()
	{
		if(m_refCount)
		{
			assert(m_refCount[0] != 0);
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

	
public:
	// Weak pointer construction
	explicit Ref(T* data, int32_t* refCount)
		: m_data(data), m_refCount(refCount)
	{
		Inc();
	}

	inline Ref() { m_data = nullptr; m_refCount = nullptr; }
	explicit inline Ref(T* obj)
	{
		m_data = obj;
		m_refCount = new int32_t(1);
	}
	inline ~Ref()
	{
		Dec();
	}
	inline Ref(const Ref& other)
	{
		m_data = other.m_data;
		m_refCount = other.m_refCount;
		Inc();
	}
	inline Ref(Ref&& other)
	{
		m_data = other.m_data;
		m_refCount = other.m_refCount;
		other.m_refCount = nullptr;
	}
	inline Ref& operator=(const Ref& other)
	{
		Dec();
		m_data = other.m_data;
		m_refCount = other.m_refCount;
		Inc();
		return *this;
	}
	inline Ref& operator=(Ref&& other)
	{
		Dec();
		m_data = other.m_data;
		m_refCount = other.m_refCount;
		other.m_refCount = nullptr;
		return *this;
	}
	inline T& operator*() { assert(IsValid()); return *m_data; }
	inline const T& operator*() const { assert(IsValid()); return *m_data; }
	inline T* operator->() { assert(IsValid()); return m_data; }
	inline const T* operator->() const { assert(IsValid()); return m_data; }

	template<typename Target>
	inline Target* Cast()
	{
		if(!m_refCount)
			return nullptr;
		return dynamic_cast<Target*>(m_data);
	}
	template<typename Target>
	inline const Target* Cast() const
	{
		if(!m_refCount)
			return nullptr;
		return dynamic_cast<Target*>(m_data);
	}
	// Casts the reference to a different type while not breaking the reference counting
	template<typename Target>
	inline Ref<Target> As()
	{
		Target* castData = dynamic_cast<Target*>(m_data);
		if(!m_refCount || !castData)
			return Ref<Target>();
		return Ref<Target>(castData, m_refCount);
	}

	inline bool operator<(const Ref& other) const
	{
		return m_refCount < other.m_refCount;
	}
	inline bool operator==(const Ref& other) const
	{
		return m_refCount == other.m_refCount;
	}
	inline bool operator!=(const Ref& other) const
	{
		return m_refCount != other.m_refCount;
	}
	inline bool operator==(const T* other) const
	{
		return m_refCount && m_data == other;
	}
	inline bool operator!=(const T* other) const
	{
		return m_refCount && m_data != other;
	}

	inline void Destroy()
	{
		assert(IsValid());
		assert(m_refCount[0] > 0);
		delete m_data;
		m_refCount[0] = -m_refCount[0] + 1;
		m_refCount = nullptr;
	}
	inline void Release()
	{
		Dec();
		m_refCount = nullptr;
	}

	inline bool IsValid() const { return m_refCount != nullptr && m_refCount[0] > 0; }
	inline operator bool() const { return IsValid(); }

	inline int32_t GetRefCount() const { if(m_refCount && m_refCount[0] > 0) return m_refCount[0]; else return 0; }

	inline T* GetData() { assert(IsValid()); return m_data; }
	inline const T* GetData() const { assert(IsValid()); return m_data; }
};

namespace Utility
{
	template<typename T>
	Ref<T> MakeRef(T* obj)
	{
		return Ref<T>(obj);
	}
}