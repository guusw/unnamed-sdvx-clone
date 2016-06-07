#pragma once
#include "Shared/String.hpp"
#include "Shared/Vector.hpp"

/* 
	File path API 
	Operates on string objects that contain path strings
	Valid formats are windows strings using \'s as directory seperators and X: as drive letters
*/
class Path
{
public:
	// Working dir
	static String GetCurrentPath();
	static String GetExecutablePath();

	static String GetTemporaryPath();
	static String GetTemporaryFileName(const String& path, const String& prefix = String());

	static bool IsDirectory(const String& path);
	static bool FileExists(const String& path);
	// Converts a path to a standard format with all duplicate slashes removed and set to the platform default '\' on windows
	static String Canonical(const String& path);
	static String Absolute(const String& path);
	static bool IsAbsolute(const String& path);
	// Removes the last directory or filename ('/' seperated)
	// returns the removed element in lastOut if set
	static String RemoveLast(const String& path, String* lastOut = nullptr);

	static String GetExtension(const String& path);
	static String ReplaceExtension(String path, String newExt);

	// Removes the first argument without spaces or within quotes from the input string and returns it
	static String ExtractPathFromCmdLine(String& input);
	// Same as ExtractPathFromCmdLine but returns the result in an array and keeps the input intact
	static Vector<String> SplitCommandLine(const String& input);
	
	// The character seperating directories/files 
	static char sep;
};