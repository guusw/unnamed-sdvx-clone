#pragma once
#include "Shared/Bindable.hpp"

/*
	Action is an objects that holds the neccesary information to call a single static function / member function / lambda
*/
template<typename R = void, typename... A>
class Action
{
public:
	Action() = default;
	// Create a static function binding
	Action(R (*staticFunction)(A ...))
	{
		m_binding = new StaticBinding<R, A...>(staticFunction);
	}
	// Create a member function binding
	template<typename C>
	Action(C* obj, R (C::*memberFunc)(A ...))
	{
		m_binding = new ObjectBinding<C, R, A...>(obj, memberFunc);
	}
	template<typename L>
	Action(L l)
	{
		m_binding = new LambdaBinding<L, R, A...>(l);
	}
	Action(Action&& other)
	{
		m_binding = other.m_binding;
		other.m_binding = nullptr;
	}
	// Create an action from a lambda
	template<typename L>
	static Action<R, A...> FromLambda(L&& lambda)
	{
		Action<R, A...> ret;
		ret.m_binding = new LambdaBinding<L, R, A...>(std::forward<L>(lambda));
		return std::move(ret);
	}
	// Create a member function binding
	template<typename C>
	static Action<R, A...> FromObject(C* object, R (C::*memberFunc)(A ...))
	{
		return Action<R, A...>(object, memberFunc);
	}
	Action& operator=(Action&& other)
	{
		Clear();
		m_binding = other.m_binding;
		other.m_binding = nullptr;
		return *this;
	}
	Action(const Action& other)
	{
		if(other.m_binding)
			m_binding = other.m_binding->Clone();
		else
			m_binding = nullptr;
	}
	Action& operator=(const Action& other)
	{
		Clear();
		if(other.m_binding)
			m_binding = other.m_binding->Clone();
		return *this;
	}
	void Bind(R (*staticFunction)(A ...))
	{
		Clear();
		m_binding = new StaticBinding<R, A...>(staticFunction);
	}
	template<typename T, typename O>
	void Bind(O* obj, R (T::*memberFunc)(A ...))
	{
		Clear();
		m_binding = new ObjectBinding<T, R, A...>(obj, memberFunc);
	}
	template<typename L>
	void BindLambda(L&& lambda)
	{
		Clear();
		m_binding = new LambdaBinding<L, R, A...>(std::forward<L>(lambda));
	}
	~Action()
	{
		Clear();
	}

	bool operator==(const Action& other) const
	{
		if(!IsBound())
			return !other.IsBound(); // (null==null) -> true

		if(!other.IsBound())
			return false;

		return m_binding->Equals(other.m_binding);
	}
	bool operator!=(const Action& other) const
	{
		return !(*this == other);
	}
	size_t GetHash() const
	{
		return m_binding->GetHash();
	}

	// NOTE: Don't implement () operator to avoid implicit casting to lambda
	R Call(A ... args)
	{
		assert(IsBound());
		return m_binding->Call(args...);
	}

	bool IsBound() const
	{
		return m_binding != nullptr;
	}
	void Clear()
	{
		if(m_binding)
			delete m_binding;
		m_binding = nullptr;
	}

private:
	IFunctionBinding<R, A...>* m_binding = nullptr;
};

namespace std
{
	template<typename R, typename... A>
	struct hash<Action<R, A...> >
	{
		size_t operator()(const Action<R, A...>& obj) const
		{
			return obj.GetHash();
		}
	};
}

/* 
	Bindable property 
	This field either acts just as a normal variable or acts a a property using Get/Set methods to interface with the underlying value
*/
template<typename T>
class Property
{
public:
	Property() = default;
	Property(T val)
		: m_value(val)
	{
	}
	Property(Action<T> get, Action<void, T> set)
		: Get(std::move(get)), Set(std::move(set))
	{
	}

	// Get
	inline operator T() const
	{
		return m_Get();
	}
	// Set
	inline Property& operator=(const T& other)
	{
		m_Set(other);
		return *this;
	}

	// Always returns the internal value
	T& GetInternalValue()
	{
		return m_value;
	}

	Action<T> Get;
	Action<void, T> Set;
protected:
	inline T m_Get() const
	{
		if(Get.IsBound())
			return const_cast<Property*>(this)->Get.Call();
		else
			return m_value;
	}
	inline void m_Set(const T& val)
	{
		if(Set.IsBound())
			Set.Call(val);
		else
			m_value = val;
	}
	T m_value;
};
