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
	static String GetCurrentPath();
	static String GetExecutablePath();

	static String GetTemporaryPath();
	static String GetTemporaryFileName(const String& path, const String& prefix = String());

	static bool FileExists(const String& path);
	static String Canonical(const String& path);
	static String Absolute(const String& path);
	static bool IsAbsolute(const String& path);
	static String RemoveLast(const String& path, String* lastOut = nullptr);

	static String GetExtension(const String& path);
	static String ReplaceExtension(String path, String newExt);

	// Removes the first argument without spaces or within quotes from the input string and returns it
	static String ExtractPathFromCmdLine(String& input);
	// Same as ExtractPathFromCmdLine but returns the result in an array and keeps the input intact
	static Vector<String> SplitCommandLine(const String& input);
};