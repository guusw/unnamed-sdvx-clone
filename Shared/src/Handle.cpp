#include "stdafx.h"
#include "Handle.hpp"

bool Handle::operator!=(const Handle& other) const
{
	return m_sharedData != other.m_sharedData;
}

bool Handle::operator==(const Handle& other) const
{
	return m_sharedData == other.m_sharedData;
}

Handle& Handle::operator=(Handle&& other)
{
	m_Dec();
	m_sharedData = other.m_sharedData;
	other.m_sharedData = nullptr;
	return *this;
}

Handle& Handle::operator=(const Handle& other)
{
	m_Dec();
	m_sharedData = other.m_sharedData;
	m_Inc();
	return *this;
}

void Handle::m_Dec()
{
	if(m_sharedData)
	{
		assert(m_sharedData[0] != 0);
		if(m_sharedData[0] >= 0)
		{
			m_sharedData[0]--;
			if(m_sharedData[0] == 0)
			{
				delete m_sharedData;
				m_sharedData = nullptr;
			}
		}
		else
		{
			m_sharedData[0]++;
			if(m_sharedData[0] == 0)
			{
				delete m_sharedData;
				m_sharedData = nullptr;
			}
		}
	}
}

void Handle::m_Inc()
{
	if(m_sharedData)
	{
		if(m_sharedData[0] >= 0)
		{
			m_sharedData[0]++;
		}
		else
		{
			m_sharedData[0]--;
		}
	}
}
Handle Handle::Create(void* userData /*= nullptr*/)
{
	Handle h;
	h.m_sharedData = new Data{ 1,userData };
	return (Handle&&)h;
}

Handle::Handle(Handle&& other)
{
	m_sharedData = other.m_sharedData;
	other.m_sharedData = nullptr;
}

Handle::Handle(const Handle& other)
{
	m_sharedData = other.m_sharedData;
	m_Inc();
}

Handle::Handle() : m_sharedData(nullptr)
{

}

Handle::~Handle()
{
	m_Dec();
}

void* Handle::GetData()
{
	assert(m_sharedData);
	return m_sharedData->data;
}
void Handle::SetData(void* userData)
{
	assert(m_sharedData);
	m_sharedData->data = userData;
}

void Handle::ForceRelease()
{
	assert(IsValid());
	assert(m_sharedData[0] > 0);
	m_sharedData->refCount = -m_sharedData->refCount + 1;
	m_sharedData = nullptr;
}

void Handle::Release()
{
	m_Dec();
	m_sharedData = nullptr;
}

int32_t Handle::GetRefCount() const
{
	if(m_sharedData && m_sharedData[0] > 0) return m_sharedData[0]; else return 0;
}

bool Handle::IsValid() const
{
	return m_sharedData != nullptr && m_sharedData[0] > 0;
}

Handle::operator bool() const
{
	return IsValid();
}
