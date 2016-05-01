#include "stdafx.h"
#include "Log.hpp"
#include "Path.hpp"
#include <ctime>

class Logger_Impl
{
public:
	Logger_Impl()
	{
		// Store the name of the executable
		moduleName = Path::GetExecutablePath();
		Path::RemoveLast(moduleName, &moduleName);
		moduleName =  Path::ReplaceExtension(moduleName, ""); // Remove .exe or dll

		// Store console output handle
		consoleHandle = GetStdHandle(STD_OUTPUT_HANDLE);
	}

	void WriteHeader(Logger::Severity severity)
	{
		// Severity strings
		const char* severityNames[] =
		{
			"Normal",
			"Warning",
			"Error",
			"Info",
		};

		// Format a timestamp string
		char timeStr[64];
		time_t currenTime = time(0);
		tm* currentLocalTime = localtime(&currenTime);
		strftime(timeStr, sizeof(timeStr), "%T", currentLocalTime);

		// Write the formated header
		Write(Utility::Sprintf("[%s][%s][%s] ", moduleName, timeStr, severityNames[(size_t)severity]));
	}
	void Write(const String& msg)
	{
		OutputDebugStringA(*msg);
	}

	HANDLE consoleHandle;
	String moduleName;
};

Logger::Logger()
{
	m_impl = new Logger_Impl;
}
Logger::~Logger()
{
	delete m_impl;
}
Logger& Logger::Get()
{
	static Logger logger;
	return logger;
}
void Logger::SetColor(Color color)
{
	if(m_impl->consoleHandle)
	{
		uint8 params[] = 
		{
			FOREGROUND_INTENSITY | FOREGROUND_RED,
			FOREGROUND_INTENSITY | FOREGROUND_GREEN,
			FOREGROUND_INTENSITY | FOREGROUND_BLUE,
			FOREGROUND_INTENSITY | FOREGROUND_BLUE | FOREGROUND_GREEN, // Yellow,
			FOREGROUND_INTENSITY | FOREGROUND_BLUE | FOREGROUND_RED, // Cyan,
			FOREGROUND_BLUE | FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_INTENSITY, // White
			FOREGROUND_BLUE | FOREGROUND_RED | FOREGROUND_GREEN, // Gray
		};
		SetConsoleTextAttribute(m_impl->consoleHandle, params[(size_t)color]);
	}
}
void Logger::Log(const String& msg, Logger::Severity severity)
{
	switch(severity)
	{
	case Normal:
		SetColor(White);
		break;
	case Info:
		SetColor(Gray);
		break;
	case Warning:
		SetColor(Yellow);
		break;
	case Error:
		SetColor(Red);
		break;
	}

	m_impl->WriteHeader(severity);
	m_impl->Write(msg);
	m_impl->Write("\n");
}

void Log(const String& msg, Logger::Severity severity)
{
	Logger::Get().Log(msg, severity);
}

String Utility::WindowsFormatMessage(DWORD code)
{
	if(code == 0)
	{
		return "No additional info available";
	}

	char buffer[1024];
	FormatMessageA(FORMAT_MESSAGE_FROM_SYSTEM, 0, code, LANG_SYSTEM_DEFAULT, buffer, sizeof(buffer), 0);
	return buffer;
}
