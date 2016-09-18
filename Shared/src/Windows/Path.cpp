#include "stdafx.h"
#include "Path.hpp"
#include "Log.hpp"
#include "Shellapi.h"

/*
	Windows version
*/
#include <windows.h>
#include <Shlwapi.h>
#pragma comment(lib, "Shlwapi.lib")

char Path::sep = '\\';

bool Path::CreateDir(const String& path)
{
	WString wpath = Utility::ConvertToWString(path);
	return CreateDirectoryW(*wpath, nullptr) == TRUE;
}
bool Path::Delete(const String& path)
{
	WString wpath = Utility::ConvertToWString(path);
	return DeleteFileW(*wpath) == TRUE;
}
bool Path::DeleteDir(const String& path)
{
	if(!ClearDir(path))
		return false;
	WString wpath = Utility::ConvertToWString(path);
	return RemoveDirectoryW(*wpath) == TRUE;
}
bool Path::Rename(const String& srcFile, const String& dstFile, bool overwrite)
{
	WString wsrc = Utility::ConvertToWString(srcFile);
	WString wdst = Utility::ConvertToWString(dstFile);
	if(PathFileExistsW(*wdst) == TRUE)
	{
		if(!overwrite)
			return false;
		if(DeleteFileW(*wdst) == FALSE)
		{
			Logf("Failed to rename file, overwrite was true but the destination could not be removed", Logger::Warning);
			return false;
		}
	}
	return MoveFileW(*wsrc, *wdst) == TRUE;
}
bool Path::Copy(const String& srcFile, const String& dstFile, bool overwrite)
{
	WString wsrc = Utility::ConvertToWString(srcFile);
	WString wdst = Utility::ConvertToWString(dstFile);
	return CopyFileW(*wsrc, *wdst, overwrite) == TRUE;
}
String Path::GetCurrentPath()
{
	char currDir[MAX_PATH];
	GetCurrentDirectoryA(sizeof(currDir), currDir);
	return currDir;
}
String Path::GetExecutablePath()
{
	char filename[MAX_PATH];
	GetModuleFileNameA(GetModuleHandle(0), filename, sizeof(filename));
	return filename;
}
String Path::GetTemporaryPath()
{
	char path[MAX_PATH];
	::GetTempPathA(sizeof(path), path);
	return path;
}
String Path::GetTemporaryFileName(const String& path, const String& prefix)
{
	char out[MAX_PATH];
	BOOL r = ::GetTempFileNameA(*path, *prefix, 0, out);
	assert(r == TRUE);
	return out;
}
bool Path::IsDirectory(const String& path)
{
	DWORD attribs = GetFileAttributesA(*path);
	return (attribs != INVALID_FILE_ATTRIBUTES) && (attribs & FILE_ATTRIBUTE_DIRECTORY) == FILE_ATTRIBUTE_DIRECTORY;
}
bool Path::FileExists(const String& path)
{
	WString wpath = Utility::ConvertToWString(path);
	return PathFileExistsW(*wpath) == TRUE;
}
String Path::Normalize(const String& path)
{
	char out[MAX_PATH];
	PathCanonicalizeA(out, *path);
	for(uint32 i = 0; i < MAX_PATH; i++)
	{
		if(out[i] == '/')
			out[i] = sep;
	}
	return out;
}
bool Path::IsAbsolute(const String& path)
{
	if(path.length() > 2 && path[1] == ':')
		return true;
	return false;
}
