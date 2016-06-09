#include "stdafx.h"
#include "Path.hpp"

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
