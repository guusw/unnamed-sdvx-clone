#pragma once
#include "Shared/Utility.hpp"
#include "Shared/Map.hpp"

template<typename... A>
struct IDelegateHandler
{
	virtual ~IDelegateHandler() {};
	virtual void Call(A... args) = 0;
	// 0 or object pointer for object handlers
	virtual void* GetObject() { return 0; }
	virtual void* GetFunction() { return 0; }
};

/* Handler for object function pointers */
template<typename Class, typename... A>
struct ObjectDelegateHandler : public IDelegateHandler<A...>
{
	ObjectDelegateHandler(Class* object, void (Class::*func)(A...)) : object(object), func(func) {};
	virtual void Call(A... args) override
	{
		(object->*func)(args...);
	}
	virtual void* GetObject() override
	{
		return object;
	}
	virtual void* GetFunction() 
	{
		return funcData;
	}

	Class* object;
	union
	{
		void (Class::*func)(A...);
		void* funcData;
	};
};
/* Handler for static functions */
template<typename... A>
struct StaticDelegateHandler : public IDelegateHandler<A...>
{
	StaticDelegateHandler(void (*func)(A...)) : func(func) {};
	virtual void Call(A... args) override
	{
		(*func)(args...);
	}
	virtual void* GetFunction()
	{
		return funcData;
	}

	union
	{
		void(*func)(A...);
		void* funcData;
	};
};

/* Handler for lambda functions */
template<typename T, typename... A>
struct LambdaDelegatehandler : public IDelegateHandler<A...>
{
	// Copies the given lambda
	LambdaDelegatehandler(T&& lambda) : lambda(lambda) {};
	virtual void Call(A... args) override
	{
		lambda(args...);
	}

	T lambda;
};

/*
	Template delegate class, can have multiple registered classes that handle a call to this function
*/
template<typename... A>
class Delegate
{
	Map<void*, IDelegateHandler<A...>*> staticMap;
	Map<void*, Map<void*, IDelegateHandler<A...>*>> objectMap;
	Map<void*, IDelegateHandler<A...>*> lambdaMap;
public:
	~Delegate()
	{
		Clear();
	}

	// Adds an object function handler
	template<typename Class>
	void Add(Class* object, void (Class::*func)(A...))
	{
		void* id = Utility::UnionCast<void*>(func);
		auto& fmap = objectMap.FindOrAdd(object);
		assert(!fmap.Contains(id));
		fmap.Add(id, new ObjectDelegateHandler<Class, A...>(object, func));
	}
	// Adds a static function handler
	void Add(void (*func)(A...))
	{
		void* id = Utility::UnionCast<void*>(func);
		assert(!staticMap.Contains(id));
		staticMap.Add(id, new StaticDelegateHandler<A...>(func));
	}
	// Adds a lambda function as a handler for this delegate
	template<typename T> void AddLambda(T&& lambda)
	{
		void* id = &lambda;
		assert(!lambdaMap.Contains(id));
		lambdaMap.Add(id, new LambdaDelegatehandler<T, A...>(std::forward<T>(lambda)));
	}

	// Removes an object handler
	template<typename Class>
	void Remove(Class* object, void(Class::*func)(A...))
	{
		void* id = Utility::UnionCast<void*>(func);
		assert(objectMap.Contains(object));
		auto& fmap = objectMap[object];
		assert(fmap.Contains(id));
		fmap.erase(id);
		if(fmap.empty())
			objectMap.erase(object);
	}
	// Removes a static handler
	void Remove(void(*func)(A...))
	{
		void* id = Utility::UnionCast<void*>(func);
		assert(staticMap.Contains(id));
		delete staticMap[id];
		staticMap.erase(id);
	}
	template<typename T> void RemoveLambda(T& lambda)
	{
		void* id = &lambda;
		assert(!lambdaMap.Contains(id));
		delete lambdaMap[id];
		lambdaMap.erase(id);
	}
	// Removes all handlers belonging to a specific object
	void RemoveAll(void* object)
	{
		auto it = objectMap.find(object);
		if(it != objectMap.end())
		{
			for(auto& f : it->second)
			{
				delete f.second;
			}
		}
		objectMap.erase(it);
	}

	// Removes all handlers
	void Clear()
	{
		// Cleanup the pointers
		for(auto& h : staticMap)
		{
			delete h.second;
		}
		for(auto& h : objectMap)
		{
			for(auto& f : h.second)
			{
				delete f.second;
			}
		}
		for(auto& h : lambdaMap)
		{
			delete h.second;
		}
		staticMap.clear();
		objectMap.clear();
		lambdaMap.clear();
	}

	// Calls the delegate
	void Call(A... args)
	{
		for(auto& h : staticMap)
		{
			h.second->Call(args...);
		}
		for(auto& h : objectMap)
		{
			for(auto& f : h.second)
			{
				f.second->Call(args...);
			}
		}
		for(auto& h : lambdaMap)
		{
			h.second->Call(args...);
		}
	}
};
