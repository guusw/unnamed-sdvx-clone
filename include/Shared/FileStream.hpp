#pragma once
#include "Shared/BinaryStream.hpp"
#include "Shared/Unique.hpp"
#include "Shared/File.hpp"

class FileStreamBase : public BinaryStream, Unique
{
protected:
	File& m_file;
public:
	FileStreamBase(File& file, bool isReading);
	virtual void Seek(size_t pos);
	virtual size_t Tell() const;
	virtual size_t GetSize() const;
	File& GetFile();
};

/* Stream that reads from a buffer */
class FileReader : public FileStreamBase
{
public:
	FileReader(File& file);
	virtual void Serialize(void* data, size_t len);
};

/* Stream that writes to a buffer */
class FileWriter : public FileStreamBase
{
public:
	FileWriter(File& file);
	virtual void Serialize(void* data, size_t len);
};