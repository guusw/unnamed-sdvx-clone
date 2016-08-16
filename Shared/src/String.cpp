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

    // UTF8 Conversion functions for linux, from http://altctrlbackspace.blogspot.nl/
#ifdef __linux__
	void utf8toWStr(WString& dest, const String& src){
     dest.clear();
     wchar_t w = 0;
     int bytes = 0;
     wchar_t err = L'�';
     for (size_t i = 0; i < src.size(); i++){
      unsigned char c = (unsigned char)src[i];
      if (c <= 0x7f){//first byte
       if (bytes){
        dest.push_back(err);
        bytes = 0;
       }
       dest.push_back((wchar_t)c);
      }
      else if (c <= 0xbf){//second/third/etc byte
       if (bytes){
        w = ((w << 6)|(c & 0x3f));
        bytes--;
        if (bytes == 0)
         dest.push_back(w);
       }
       else
        dest.push_back(err);
      }
      else if (c <= 0xdf){//2byte sequence start
       bytes = 1;
       w = c & 0x1f;
      }
      else if (c <= 0xef){//3byte sequence start
       bytes = 2;
       w = c & 0x0f;
      }
      else if (c <= 0xf7){//3byte sequence start
       bytes = 3;
       w = c & 0x07;
      }
      else{
       dest.push_back(err);
       bytes = 0;
      }
     }
     if (bytes)
      dest.push_back(err);
    }

    void wstrToUtf8(String& dest, const WString& src){
     dest.clear();
     for (size_t i = 0; i < src.size(); i++){
      wchar_t w = src[i];
      if (w <= 0x7f)
       dest.push_back((char)w);
      else if (w <= 0x7ff){
       dest.push_back(0xc0 | ((w >> 6)& 0x1f));
       dest.push_back(0x80| (w & 0x3f));
      }
      else if (w <= 0xffff){
       dest.push_back(0xe0 | ((w >> 12)& 0x0f));
       dest.push_back(0x80| ((w >> 6) & 0x3f));
       dest.push_back(0x80| (w & 0x3f));
      }
      else if (w <= 0x10ffff){
       dest.push_back(0xf0 | ((w >> 18)& 0x07));
       dest.push_back(0x80| ((w >> 12) & 0x3f));
       dest.push_back(0x80| ((w >> 6) & 0x3f));
       dest.push_back(0x80| (w & 0x3f));
      }
      else
       dest.push_back('?');
     }
    }
#endif

	String ConvertToUTF8(const WString& wstr)
	{
#ifdef _WIN32
		String res;
		const wchar_t* src = *wstr;
		int32 srcLen = (int32)wstr.length();
		uint32 dstLen = WideCharToMultiByte(CP_UTF8, 0, src, srcLen, 0, 0, nullptr, nullptr);
		char* dst = new char[dstLen];
		WideCharToMultiByte(CP_UTF8, 0, src, srcLen, dst, dstLen, nullptr, nullptr);
		res = String(dst, dst + dstLen);
		delete[] dst;
		return res;
#else
		String res;
		wstrToUtf8(res, wstr);
		return res;
#endif
	}
	WString ConvertToWString(const String& utf8String)
	{
#ifdef _WIN32
		WString res;
		const char* src = *utf8String;
		int32 srcLen = (int32)utf8String.length();
		uint32 dstLen = MultiByteToWideChar(CP_UTF8, 0, src, srcLen, 0, 0);
		wchar_t* dst = new wchar_t[dstLen];
		MultiByteToWideChar(CP_UTF8, 0, src, srcLen, dst, dstLen);
		res = WString(dst, dst + dstLen);
		delete[] dst;
		return res;
#else
		WString res;
		utf8toWStr(res, utf8String);
		return res;
#endif
	}
}
