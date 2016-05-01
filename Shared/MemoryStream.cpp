#include "stdafx.h"
#include "MemoryStream.hpp"

MemoryStreamBase::MemoryStreamBase(Buffer& buffer, bool isReading) : BinaryStream(isReading), m_buffer(buffer)
{
}
void MemoryStreamBase::Seek(size_t pos)
{
	assert(pos <= m_buffer.size());
	m_cursor = pos;
}
size_t MemoryStreamBase::Tell() const
{
	return m_buffer.size();
}
size_t MemoryStreamBase::GetSize() const
{
	return m_buffer.size();
}

MemoryReader::MemoryReader(Buffer& buffer) : MemoryStreamBase(buffer, true)
{
}
void MemoryReader::Serialize(void* data, size_t len)
{
	size_t endPos = m_cursor + len;
	assert(endPos <= m_buffer.size());
	memcpy(data, &m_buffer[m_cursor], len);
	m_cursor += len;
}

MemoryWriter::MemoryWriter(Buffer& buffer) : MemoryStreamBase(buffer, false)
{
}
void MemoryWriter::Serialize(void* data, size_t len)
{
	size_t newSize = max(m_buffer.size(), m_cursor + len);
	// Reserve space for new data
	if(newSize > m_buffer.size())
	{
		m_buffer.resize(newSize);
	}
	memcpy(&m_buffer[m_cursor], data, len);
	m_cursor += len;
}
