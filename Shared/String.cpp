#include "stdafx.h"
#include "String.hpp"
#include <cwchar>

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
		const wchar_t* src = *unicodeString;
		size_t i = 0;
		size_t max = unicodeString.length();
		while(i < max)
		{
			static char buffer[8];
			int32 converted = 0;
			wctomb_s(&converted, buffer, sizeof(buffer), *src);
			for(int32 j = 0; j < converted; j++)
				res.push_back(buffer[j]);
			src++;
			i++;
		}
		return res;
	}
	WString ConvertToUnicode(const String& ansiString)
	{
		// I'm using the windows convert function here because converting individual characters with mbtowc didn't convert utf-8 correctly
		WString res;
		const char* src = *ansiString;
		int32 srcLen = (int32)ansiString.length();
		uint32 dstLen = MultiByteToWideChar(CP_UTF8, 0, src, srcLen, 0, 0);
		wchar_t* dst = new wchar_t[dstLen];
		MultiByteToWideChar(CP_UTF8, 0, src, srcLen, dst, dstLen);
		res = WString(dst, dst + dstLen);
		delete[] dst;
		return res;
	}

}