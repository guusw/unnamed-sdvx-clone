#pragma once
#include "Shared/BinaryStream.hpp"
#include "Shared/Unique.hpp"
#include "Shared/Buffer.hpp"

class MemoryStreamBase : public BinaryStream, Unique
{
protected:
	Buffer& m_buffer;
	size_t m_cursor = 0; // Write position
public:
	MemoryStreamBase(Buffer& buffer, bool isReading);
	virtual void Seek(size_t pos);
	virtual size_t Tell() const;
	virtual size_t GetSize() const;
};

/* Stream that reads from a buffer */
class MemoryReader : public MemoryStreamBase
{
public:
	MemoryReader(Buffer& buffer);
	virtual void Serialize(void* data, size_t len);
};

/* Stream that writes to a buffer */
class MemoryWriter : public MemoryStreamBase
{
public:
	MemoryWriter(Buffer& buffer);
	virtual void Serialize(void* data, size_t len);
};