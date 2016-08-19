#include "stdafx.h"
#include "String.hpp"

// UTF8 Conversion from http://altctrlbackspace.blogspot.nl/2009/03/c-utf8-to-wstring-conversion-routine.html
void utf8toWStr(WString& dest, const String& src);
void wstrToUtf8(String& dest, const WString& src);

namespace Utility
{
	const char* SprintfArgFilter(const String& in)
	{
		return *in;
	}
	const wchar_t* WSprintfArgFilter(const WString& in)
	{
		return *in;
	}
	String ConvertToUTF8(const WString& wstr)
	{
		String res;
		wstrToUtf8(res, wstr);
		return res;
	}
	WString ConvertToWString(const String& utf8String)
	{
		WString res;
		utf8toWStr(res, utf8String);
		return res;
	}
}
