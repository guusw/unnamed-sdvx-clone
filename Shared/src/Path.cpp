#include "stdafx.h"
#include "Path.hpp"

#ifdef _WIN32
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
	return PathFileExistsA(*path) == TRUE;
}
String Path::Canonical(const String& path)
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
#else
/*
	Linux version
*/
#include <unistd.h>
#include <linux/limits.h>
#include <sys/stat.h>
// Convenience
#define MAX_PATH PATH_MAX

char Path::sep = '/';

String Path::GetCurrentPath()
{
	char currDir[MAX_PATH];
	getcwd(currDir, MAX_PATH);
	return currDir;
}
String Path::GetExecutablePath()
{
	char filename[MAX_PATH];
	
	pid_t pid = getpid();
	
	// Get name from pid
	char path[MAX_PATH];
	sprintf(path, "/proc/%d/exe", pid);
	int r = readlink(path, filename, PATH_MAX);
	assert(r != -1);
	
	return filename;
}
String Path::GetTemporaryPath()
{
	char out[MAX_PATH];
	String curr = GetCurrentPath();
	sprintf(out, "%s/tempXXXXXX", *curr);
	mktemp(out);
	return out;
}
String Path::GetTemporaryFileName(const String& path, const String& prefix)
{
	char out[MAX_PATH];
	sprintf(out, "%s/%sXXXXXX", *path, *prefix);
	mktemp(out);
	return out;
}

bool Path::IsDirectory(const String& path)
{
	struct stat sb;
    if (stat(*path, &sb) == 0 && S_ISDIR(sb.st_mode))
		return true;
	return false;
}

bool Path::FileExists(const String& path)
{
	return access(*path, F_OK) != -1;
}
String Path::Canonical(const String& path)
{
	char out[MAX_PATH];
	realpath(*path, out);
	for(uint32 i = 0; i < MAX_PATH; i++)
	{
		if(out[i] == '\\')
			out[i] = sep;
	}
	return out;
}
bool Path::IsAbsolute(const String& path)
{
	if(path.length() > 1 && path[0] == '/')
		return true;
	return false;
}
#endif

/* 
	Common 
*/
String Path::Absolute(const String& path)
{
	if(IsAbsolute(path))
		return path;

	String currentDir = GetCurrentPath();
	return currentDir + sep + path;
}
String Path::RemoveLast(const String& path, String* lastOut /*= nullptr*/)
{
	String left;
	if(path.SplitLast(String() + sep, &left, lastOut))
		return left;
	return path;
}

String Path::GetExtension(const String& path)
{
	size_t dotPos = path.find(".");
	if(dotPos == -1)
		return String();
	return path.substr(dotPos + 1);
}
String Path::ReplaceExtension(String path, String newExt)
{
	newExt.TrimFront('.');

	// Remove everything in the extension and the dot
	size_t dotPos = path.find(".");
	if(dotPos != -1)
	{
		path.erase(path.begin() + dotPos, path.end());
	}

	if(newExt.empty())
		return path;

	path.push_back('.');
	path += newExt;

	return path;
}
String Path::ExtractPathFromCmdLine(String& input)
{
	String r;
	input.Trim();
	bool q = input[0] == '"';
	size_t i = q ? 1 : 0;
	while(i < input.size())
	{
		if(q)
		{
			if(input[i] == '"')
			{
				break;
			}
		}
		else
		{
			if(input[i] == ' ')
			{
				break;
			}
		}
		r += input[i];
		i++;
	}
	input.erase(input.begin(), input.begin() + r.size() + (q ? 2 : 0));
	input.Trim();
	return r;
}
Vector<String> Path::SplitCommandLine(const String& input)
{
	String buffer = input;
	Vector<String> res;
	while(!buffer.empty())
	{
		String a = ExtractPathFromCmdLine(buffer);
		a.Trim();
		if(!a.empty())
			res.push_back(a);
	}
	return res;
}
