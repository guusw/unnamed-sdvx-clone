#pragma once
#include "Shared/String.hpp"
#include "Shared/Vector.hpp"

enum class FileType
{
	Regular = 0,
	Folder
};

/* 
	Result of file finding operations
*/
struct FileInfo
{
	String fullPath;
	uint64 lastWriteTime;
	FileType type;

};

/*
	File enumeration functions
*/
class Files
{
public:
	// Finds files in a given folder
	// uses the given extension filter if specified
	static Vector<FileInfo> ScanFiles(const String& folder, String extFilter = String());

	// Finds files in a given folder, recursively
	// uses the given extension filter if specified
	static Vector<FileInfo> ScanFilesRecursive(const String& folder, String extFilter = String());
};