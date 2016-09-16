#include "stdafx.h"
#include "Path.hpp"

/*
	Windows version
*/
#include <windows.h>
#include <Shlwapi.h>
#pragma comment(lib, "Shlwapi.lib")

char Path::sep = '\\';

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

String Path::GetModuleName()
{
	String moduleName = Path::GetExecutablePath();
	Path::RemoveLast(moduleName, &moduleName);
	return moduleName;
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
