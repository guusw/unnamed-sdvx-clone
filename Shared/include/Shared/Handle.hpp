#pragma once
#include <assert.h>

/*
	Similar to a Ref object but this one is just for 
	holding a handle to something which can be released and checked for that
	Initialize invalid, use Handle::Create() to create a valid handle
*/
class Handle
{
private:
	struct Data
	{
		int32 refCount;
		void* data;
		operator int32&() { return refCount; }
	};
	Data* m_sharedData;
	void m_Dec();
	void m_Inc();

public:
	// Create a new initialized handle
	static Handle Create(void* userData = nullptr);

public:
	Handle();
	~Handle();
	Handle(const Handle& other);
	Handle(Handle&& other);
	Handle& operator=(const Handle& other);
	Handle& operator=(Handle&& other);

	bool operator==(const Handle& other) const;
	bool operator!=(const Handle& other) const;

	// User Data
	void* GetData();
	template<typename T>
	T* GetData() { return (T*)GetData(); }

	void SetData(void* userData);

	// Destroy the handle, all copies of this handle will become invalid
	// Can only be used on a valid handle
	void ForceRelease();

	// Release the handle, releasing this handle
	void Release();

	// The number copies of this Handle object, if it has been initialized
	int32_t GetRefCount() const;

	// Checks if the handle has been initialized
	bool IsValid() const;
	operator bool() const;
};