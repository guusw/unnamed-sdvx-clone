#include "stdafx.h"
#include "String.hpp"

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

	String ConvertToANSI(const WString& unicodeString)
	{
		String res;
		res.resize(wcstombs(NULL, unicodeString.c_str(), 0));
		wcstombs(res.GetData(), unicodeString.c_str(), res.size());
		return res;
	}
	WString ConvertToUnicode(const String& ansiString)
	{
		WString res;
		res.resize(mbstowcs(nullptr, ansiString.c_str(), 0));
		mbstowcs(res.GetData(), ansiString.c_str(), res.size());
		return res;
	}

}