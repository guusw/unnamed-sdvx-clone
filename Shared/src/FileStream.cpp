#include "stdafx.h"
#include "FileStream.hpp"

FileStreamBase::FileStreamBase(File& file, bool isReading) : m_file(&file), BinaryStream(isReading)
{
}
void FileStreamBase::Seek(size_t pos)
{
	assert(m_file);
	m_file->Seek(pos);
}
size_t FileStreamBase::Tell() const
{
	assert(m_file);
	return m_file->Tell();
}
size_t FileStreamBase::GetSize() const
{
	assert(m_file);
	return m_file->GetSize();
}
File& FileStreamBase::GetFile()
{
	assert(m_file);
	return *m_file;
}

FileReader::FileReader(File& file) : FileStreamBase(file, true)
{

}
size_t FileReader::Serialize(void* data, size_t len)
{
	assert(m_file);
	return m_file->Read(data, len);
}

FileWriter::FileWriter(File& file) : FileStreamBase(file, false)
{
}
size_t FileWriter::Serialize(void* data, size_t len)
{
	assert(m_file);
	return m_file->Write(data, len);
}
