#include "stdafx.h"
#include "Path.hpp"

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
String Path::Normalize(const String& path)
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
