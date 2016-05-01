#pragma once
#include <type_traits>
#include "Shared/Vector.hpp"
#include "Shared/Map.hpp"
#include "Shared/String.hpp"

/*
	Abstract binary stream base class
	a stream can either be operating as a reading stream or as a writing stream
*/
class BinaryStream
{
protected:
	BinaryStream(bool isReading = false)
	{
		m_isReading = isReading;
	}
public:
	// Reads or writes a struct or native type's data based on the stream's mode of operation
	template<typename T>
	typename std::enable_if<!std::is_pointer<T>::value, bool>::type SerializeObject(T& obj)
	{
		static_assert(std::is_trivially_copyable<T>::value == true, "Type is not trivially copyable");
		Serialize(&obj, sizeof(obj));
		return true;
	}
	// Writes or reads a pointer type by calling StaticSerialize on the type
	template <typename T>
	typename std::enable_if<!std::is_void<decltype(T::StaticSerialize(*(BinaryStream*)0, *(T**)0))>::value, bool>::type SerializeObject(T*& obj)
	{
		return T::StaticSerialize(*this, obj);
	}
	template <typename T>
	typename std::enable_if<std::is_pointer<T>::value, bool>::type SerializeObject(T& obj)
	{
		static_assert(false, "Can't serialize pointer, no static SerializeObject found");
	}
	template<>
	bool SerializeObject(String& obj)
	{
		*this << obj;
		return true;
	}
	template<>
	bool SerializeObject(WString& obj)
	{
		*this << obj;
		return true;
	}

	// Reads or writes data based on the stream's mode of operation
	virtual void Serialize(void* data, size_t len) = 0;
	// Seeks to a position in the stream
	virtual void Seek(size_t pos) = 0;
	// Tells the position of the stream
	virtual size_t Tell() const = 0;
	// Returns the current size of the stream
	//	either the max amount of readable data or the amount of currently written data
	virtual size_t GetSize() const = 0;

	// Stream operators
#define DECLARE_STREAMABLE_TYPE(_type)\
	BinaryStream& operator<<(_type& obj);

	DECLARE_STREAMABLE_TYPE(char);
	DECLARE_STREAMABLE_TYPE(float);
	DECLARE_STREAMABLE_TYPE(double);
	DECLARE_STREAMABLE_TYPE(uint8);
	DECLARE_STREAMABLE_TYPE(uint16);
	DECLARE_STREAMABLE_TYPE(uint32);
	DECLARE_STREAMABLE_TYPE(uint64);
	DECLARE_STREAMABLE_TYPE(int8);
	DECLARE_STREAMABLE_TYPE(int16);
	DECLARE_STREAMABLE_TYPE(int32);
	DECLARE_STREAMABLE_TYPE(int64);
	DECLARE_STREAMABLE_TYPE(String);
	DECLARE_STREAMABLE_TYPE(WString);

#undef DECLARE_STREAMABLE_TYPE

	template<typename T>
	BinaryStream& operator<<(Vector<T>& obj);
	template<typename K, typename V>
	BinaryStream& operator<<(Map<K, V>& obj);

	bool IsReading() const 
	{
		return m_isReading;
	}
	bool IsWriting() const
	{
		return !m_isReading;
	}
protected:
	bool m_isReading;
};

template<typename T>
BinaryStream& BinaryStream::operator<<(Vector<T>& obj)
{
	if(IsReading())
	{
		obj.clear();
		uint32 len;
		*this << len; 
		for(uint32 i = 0; i < len; i++)
		{
			T v;
			bool ok = SerializeObject(v);
			assert(ok);
			obj.Add(v);
		}
	}
	else
	{
		uint32 len;
		*this << len;
		for(uint32 i = 0; i < len; i++)
		{
			bool ok = SerializeObject(obj[v]);
			assert(ok);
		}
	}
	return *this;
}
template<typename K, typename V>
BinaryStream& BinaryStream::operator<<(Map<K, V>& obj)
{
	if(IsReading())
	{
		obj.clear();
		uint32 len;
		*this << len;
		for(uint32 i = 0; i < len; i++)
		{
			K k;
			V v;
			bool ok = true;
			ok = ok && SerializeObject(k);
			ok = ok && SerializeObject(v);
			assert(ok);
			obj.Add(k, v);
		}
	}
	else
	{
		uint32 len = (uint32)obj.size();
		*this << len;
		for(auto& p : obj)
		{
			bool ok = true;
			ok = ok && SerializeObject(const_cast<K&>(p.first));
			ok = ok && SerializeObject(p.second);
			assert(ok);
		}
	}
	return *this;
}
