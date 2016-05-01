#include "stdafx.h"
#include "FileStream.hpp"

FileStreamBase::FileStreamBase(File& file, bool isReading) : m_file(file), BinaryStream(isReading)
{
}
void FileStreamBase::Seek(size_t pos)
{
	m_file.Seek(pos);
}
size_t FileStreamBase::Tell() const
{
	return m_file.Tell();
}
size_t FileStreamBase::GetSize() const
{
	return m_file.GetSize();
}
File& FileStreamBase::GetFile()
{
	return m_file;
}

FileReader::FileReader(File& file) : FileStreamBase(file, true)
{

}
void FileReader::Serialize(void* data, size_t len)
{
	m_file.Read(data, len);
}

FileWriter::FileWriter(File& file) : FileStreamBase(file, false)
{
}
void FileWriter::Serialize(void* data, size_t len)
{
	m_file.Write(data, len);
}
