#include "stdafx.h"
#include "ConfigEntry.hpp"

String IntConfigEntry::ToString() const
{
	static char buffer[32] = { 0 };
	return itoa(data, buffer, 10);
}
void IntConfigEntry::FromString(const String& str)
{
	data = atoi(*str);
}

String BoolConfigEntry::ToString() const
{
	return data ? "True" : "False";
}
void BoolConfigEntry::FromString(const String& str)
{
	data = (str == "True");
}

String StringConfigEntry::ToString() const
{
	return "\"" + data + "\"";
}
void StringConfigEntry::FromString(const String& str)
{
	data = str;
	data.Trim('"');
}

String FloatConfigEntry::ToString() const
{
	return Utility::Sprintf("%f", data);
}
void FloatConfigEntry::FromString(const String& str)
{
	data = (float)atof(*str);
}
