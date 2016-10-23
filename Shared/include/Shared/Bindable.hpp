#pragma once
#include "Shared/Utility.hpp"
#include "Shared/Map.hpp"

template<typename R, typename... A>
struct IFunctionBinding
{
	virtual ~IFunctionBinding() {};
	virtual R Call(A... args) = 0;
	virtual IFunctionBinding* Clone() = 0;
	virtual bool Equals(const IFunctionBinding* other) const = 0;
	virtual size_t GetHash() const = 0;
};

/* Member function binding */
template<typename Class, typename R, typename... A>
struct ObjectBinding : public IFunctionBinding<R, A...>
{
	ObjectBinding(Class* object, R (Class::*func)(A...)) : object(object), func(func) {};
	virtual R Call(A... args) override
	{
		return ((object)->*func)(args...);
	}
	virtual IFunctionBinding* Clone()
	{
		return new ObjectBinding(object, func);
	}
	virtual bool Equals(const IFunctionBinding* other) const 
	{
		assert(other);
		auto obj2 = dynamic_cast<const ObjectBinding<Class, R, A...>*>(other);
		if(!obj2)
			return false;
		return object == obj2->object && func == obj2->func;
	}
	virtual size_t GetHash() const
	{
		union
		{
			R(Class::*funcPtr)(A...);
			size_t funcInt;
		};
		funcPtr = func;

		std::size_t seed = 0x72998EDF;
		seed ^= (seed << 6) + (seed >> 2) + 0x45218C23 + (size_t)object;
		seed ^= (seed << 6) + (seed >> 2) + 0x6C04D06E + funcInt;
		return seed;
	}

	Class* object;
	R (Class::*func)(A...);
};

/* Static function binding */
template<typename R, typename... A>
struct StaticBinding : public IFunctionBinding<R, A...>
{
	StaticBinding(R(*func)(A...)) : func(func) {};
	virtual R Call(A... args) override
	{
		return (*func)(args...);
	}
	virtual IFunctionBinding* Clone()
	{
		return new StaticBinding(func);
	}
	virtual bool Equals(const IFunctionBinding* other) const
	{
		assert(other);
		auto obj2 = dynamic_cast<const StaticBinding<R, A...>*>(other);
		if(!obj2)
			return false;
		return func == obj2->func;
	}
	virtual size_t GetHash() const
	{
		union
		{
			R(*funcPtr)(A...);
			size_t funcInt;
		};
		funcPtr = func;

		std::size_t seed = 0x67DEC83B;
		seed ^= (seed << 6) + (seed >> 2) + 0x609F73AC + funcInt;
		return seed;
	}

	R(*func)(A...);
};

/* Lambda function binding */
template<typename T, typename R, typename... A>
struct LambdaBinding : public IFunctionBinding<R, A...>
{
	// Copies the given lambda
	LambdaBinding(const T& lambda) 
		: lambda(lambda) 
	{
	}
	virtual R Call(A... args) override
	{
		return lambda(args...);
	}
	virtual IFunctionBinding* Clone()
	{
		return new LambdaBinding(lambda);
	}
	virtual bool Equals(const IFunctionBinding* other) const
	{
		// Can't compare lambdas
		return false;
	}
	virtual size_t GetHash() const
	{
		return 0;
	}

	T lambda;
};

/* Constant return value binding */
template<typename R, typename... A>
struct ConstantBinding : public IFunctionBinding<R, A...>
{
	ConstantBinding(const R& value) : value(value) {};
	virtual R Call(A...) override
	{
		return value;
	}
	virtual IFunctionBinding* Clone()
	{
		return new ConstantBinding(value);
	}
	virtual bool Equals(const IFunctionBinding* other) const
	{
		assert(other);
		auto obj2 = dynamic_cast<const ConstantBinding<R, A...>*>(other);
		if(!obj2)
			return false;
		return obj2->value == value;
	}
	virtual size_t GetHash() const
	{
		// Don't hash constant bindings to avoid the need for all constant binding types to have a hash function
		return 1;
	}

	R value;
};