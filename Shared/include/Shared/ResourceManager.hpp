#pragma once
#include "Shared/String.hpp"
#include "Shared/Vector.hpp"
#include "Shared/Ref.hpp"
#include "Shared/TypeInfo.hpp"
#include "Shared/Unique.hpp"
#include "Shared/Log.hpp"
#include "Shared/Thread.hpp"

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
	struct ResourceManagerObject
	{
		// Reference to the object
		Ref<T> ref;
		// Timer to keep track of when the resource was last used
		Timer unusedDuration;
		// name of the object, if set
		String name;
	};

	// List of managed object
	Vector<ResourceManagerObject*> m_objects;
	Map<String, ResourceManagerObject*> m_objectsByName;
	Mutex m_lock;
public:
	ResourceManager()
	{
	}
	~ResourceManager()
	{
		ReleaseAll();
	}

	// Finds a loaded object by name
	Ref<T> Find(const String& name)
	{
		auto found = m_objectsByName.Find(name);
		return found ? (*found)->ref : Ref<T>();
	}

	// Creates a new reference counted object to this object and returns it
	// when the object is no longer referenced the resource manager will collect it when the next garbage collection triggers
	const Ref<T> Register(T* pObject, const String& name = String())
	{
		Ref<T> ret = Utility::MakeRef(pObject);
		m_lock.lock();
		auto obj = m_objects.Add(new ResourceManagerObject());
		obj->name = name;
		obj->ref = ret;
		if(!name.empty())
		{
			// Must only exist once
			assert(!m_objectsByName.Contains(name));
			m_objectsByName.Add(name, obj);
		}

		m_lock.unlock();
		return ret;
	}

	// Finds a loaded object by name
	virtual void GarbageCollect() override
	{
		size_t numCleanedUp = 0;
		m_lock.lock();
		for(auto it = m_objects.begin(); it != m_objects.end();)
		{
			auto object = *it;
			if(object->ref.GetRefCount() <= 1)
			{
				// Cleanup after timer expired
				if(object->unusedDuration.Milliseconds() > keepAliveDuration)
				{
					numCleanedUp++;
					m_UnloadObject(object);
					it = m_objects.erase(it);
					break; // Avoid unloading everything at the same time
				}
			}
			else
			{
				// Restart usage timer since object is still used
				object->unusedDuration.Restart();
			}

			it++;
		}
		m_lock.unlock();
	}

	virtual void ReleaseAll()
	{
		m_lock.lock();
		size_t numCleanedUp = m_objects.size();
		for(auto it = m_objects.begin(); it != m_objects.end(); it++)
		{
			m_UnloadObject(*it);
		}
		m_objects.clear();
		assert(m_objectsByName.empty());
		m_lock.unlock();
	}

	// The duration to keep objects alive after they are no longer used, in milliseconds 
	uint32 keepAliveDuration = 10000;

private:
	void m_UnloadObject(ResourceManagerObject* object)
	{
		if(object->ref)
			object->ref.Destroy();
		if(!object->name.empty())
			m_objectsByName.erase(object->name);
		delete object;
	}
};