#pragma once
#include "Shared/String.hpp"
#include "Shared/Vector.hpp"
#include "Shared/Ref.hpp"
#include "Shared/TypeInfo.hpp"
#include "Shared/Unique.hpp"
#include "Shared/Log.hpp"

class IResourceManager
{
public:
	// Collects unused object
	virtual void GarbageCollect() = 0;
	// Forcefully releases all objects from this resource manager
	virtual void ReleaseAll() = 0;
	virtual ~IResourceManager() = default;
};

/*
	Templated resource managed that keeps Ref<> objects
	the GarbageCollect function checks these and cleans up unused ones
*/
template<typename T>
class ResourceManager : public IResourceManager, Unique
{
	// List of managed object
	Vector<Ref<T>> m_objects;
public:
	ResourceManager()
	{
	}
	~ResourceManager()
	{
	}
	// Creates a new reference counted object to this object and returns it
	// when the object is no longer referenced the resource manager will collect it when the next garbage collection triggers
	const Ref<T> Register(T* pObject)
	{
		Ref<T> ret = Utility::MakeRef(pObject);
		m_objects.push_back(ret);
		return ret;
	}
	virtual void GarbageCollect() override
	{
		size_t numCleanedUp = 0;
		for(auto it = m_objects.begin(); it != m_objects.end();)
		{
			if(it->GetRefCount() == 1)
			{
				numCleanedUp++;
				it = m_objects.erase(it);
				continue;
			}
			it++;
		}
		if(numCleanedUp > 0)
		{
			Logf("Cleaned up %d resource(s) of %s", Logger::Info, numCleanedUp, Utility::TypeInfo<T>::name);
		}
	}
	virtual void ReleaseAll()
	{
		size_t numCleanedUp = m_objects.size();
		for(auto it = m_objects.begin(); it != m_objects.end(); it++)
		{
			it->Destroy();
		}
		m_objects.clear();
		if(numCleanedUp > 0)
		{
			Logf("Cleaned up %d resource(s) of %s", Logger::Info, numCleanedUp, Utility::TypeInfo<T>::name);
		}
	}
};