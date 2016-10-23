#pragma once
#include <Shared/Action.hpp>

/*
	MarkDirty field
	Calls an action when the field's value changed
*/
template <typename T>
class NotifyDirty
{
public:
	NotifyDirty() = default;
	NotifyDirty(T val)
		: m_value(val)
	{
	}
	// Accessors
	operator const T&() const
	{
		return m_value;
	}
	const T& Get() const
	{
		return m_value;
	}
	const T* operator->() const
	{
		return &m_value;
	}
	// Implicit set, this marks the field dirty
	NotifyDirty& operator=(const T& other)
	{
		m_Set(other);
		return *this;
	}
	// Comparison
	bool operator==(const NotifyDirty& rhs) const
	{
		return m_value == rhs.m_value;
	}
	bool operator!=(const NotifyDirty& rhs) const
	{
		return m_value != rhs.m_value;
	}
	Action<void> Notify;
protected:
	void m_Set(const T& val)
	{
		if(Notify.IsBound())
			Notify.Call();
		m_value = val;
	}
	T m_value;
};

/*
	Represents a cached value, can be invalidated which will require someone to update it using Update()
	when it is updated IsValid will return true again until invalidated again
*/
template<typename T>
class Cached
{
public:
	Cached(bool initiallyValid = false) : m_valid(initiallyValid) {}
	Cached(const Cached& other)
		: m_valid(false),
		  m_value(other.m_value)
	{
	}
	Cached& operator=(const Cached& other)
	{
		if(this == &other)
			return *this;
		m_valid = false;
		m_value = other.m_value;
		return *this;
	}
	bool IsValid() const
	{
		return m_valid;
	}
	void Invalidate()
	{
		m_valid = false;
	}
	// Explicit update
	void Update(const T& newValue)
	{
		m_value = newValue;
		m_valid = true;
	}
	// Implicit update
	Cached<T>& operator=(const T& other)
	{
		Update(other);
		return *this;
	}
	// Accessors
	operator const T&() const
	{
		return m_value;
	}
	const T& Get() const
	{
		return m_value;
	}
	const T* operator->() const
	{
		return &m_value;
	}
	// Comparison
	bool operator==(const Cached& rhs) const
	{
		return m_value == rhs.m_value;
	}
	bool operator!=(const Cached& rhs) const
	{
		return m_value != rhs.m_value;
	}
private:
	bool m_valid = false;
	T m_value;
};

/*
	Same as templated Cached but without an actual value
*/
template<>
class Cached<void>
{
public:
	Cached(bool initiallyValid = false) : m_valid(initiallyValid) {}
	Cached(const Cached& other) : m_valid(false)
	{
	}
	Cached& operator=(const Cached& other)
	{
		if(this == &other)
			return *this;
		m_valid = false;
		return *this;
	}
	bool IsValid() const
	{
		return m_valid;
	}
	void Invalidate()
	{
		m_valid = false;
	}
	// Explicit update
	void Update()
	{
		m_valid = true;
	}
private:
	bool m_valid = false;
};

// Cached<void>
typedef Cached<void> CachedState;