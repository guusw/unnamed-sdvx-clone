#include "stdafx.h"
#include "String.hpp"
#include <locale>
#include <clocale>

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

	String ConvertToUTF8(const WString& unicodeString)
	{
#ifdef _WIN32
		String res;
		const wchar_t* src = *unicodeString;
		int32 srcLen = (int32)unicodeString.length();
		uint32 dstLen = WideCharToMultiByte(CP_UTF8, 0, src, srcLen, 0, 0, nullptr, nullptr);
		char* dst = new char[dstLen];
		WideCharToMultiByte(CP_UTF8, 0, src, srcLen, dst, dstLen, nullptr, nullptr);
		res = String(dst, dst + dstLen);
		delete[] dst;
		return res;
#else
		String res;
		res.resize(wcstombs(NULL, unicodeString.c_str(), 0));
		std::wcstombs(res.GetData(), unicodeString.c_str(), res.size());
		return res;
#endif
	}
	WString ConvertToUnicode(const String& ansiString)
	{
#ifdef _WIN32
		WString res;
		const char* src = *ansiString;
		int32 srcLen = (int32)ansiString.length();
		uint32 dstLen = MultiByteToWideChar(CP_UTF8, 0, src, srcLen, 0, 0);
		wchar_t* dst = new wchar_t[dstLen];
		MultiByteToWideChar(CP_UTF8, 0, src, srcLen, dst, dstLen);
		res = WString(dst, dst + dstLen);
		delete[] dst;
		return res;
#else
		WString res;
		res.resize(mbstowcs(nullptr, ansiString.c_str(), 0));
		std::mbstowcs(res.GetData(), ansiString.c_str(), res.size());
		return res;
#endif
	}

}