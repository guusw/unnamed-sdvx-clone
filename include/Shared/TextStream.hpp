#pragma once
#include "Shared/BinaryStream.hpp"

class TextStream
{
public:
	static bool ReadLine(BinaryStream& stream, String& out, const String& lineEnding = "\r\n");
	static void Write(BinaryStream& stream, const String& out);
	static void WriteLine(BinaryStream& stream, const String& out, const String& lineEnding = "\r\n");
};