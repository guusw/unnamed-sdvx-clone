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

#ifdef _WIN32
		// Store console output handle
		consoleHandle = GetStdHandle(STD_OUTPUT_HANDLE);
#endif
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
		time_t currentTime = time(0);
		tm* currentLocalTime = localtime(&currentTime);
		strftime(timeStr, sizeof(timeStr), "%T", currentLocalTime);

		// Write the formated header
		Write(Utility::Sprintf("[%s][%s][%s] ", moduleName, timeStr, severityNames[(size_t)severity]));
	}
	void Write(const String& msg)
	{
#ifdef _WIN32
		OutputDebugStringA(*msg);
#else
		printf("%s", msg.c_str());
#endif
	}

#ifdef _WIN32
	HANDLE consoleHandle;
#endif
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
#ifdef _WIN32
	if(m_impl->consoleHandle)
	{
		uint8 params[] = 
		{
			FOREGROUND_INTENSITY | FOREGROUND_RED,
			FOREGROUND_INTENSITY | FOREGROUND_GREEN,
			FOREGROUND_INTENSITY | FOREGROUND_BLUE,
			FOREGROUND_INTENSITY | FOREGROUND_BLUE | FOREGROUND_GREEN, // Yellow,
			FOREGROUND_INTENSITY | FOREGROUND_BLUE | FOREGROUND_RED, // Cyan,
			FOREGROUND_INTENSITY | FOREGROUND_GREEN | FOREGROUND_RED, // Magenta,
			FOREGROUND_BLUE | FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_INTENSITY, // White
			FOREGROUND_BLUE | FOREGROUND_RED | FOREGROUND_GREEN, // Gray
		};
		SetConsoleTextAttribute(m_impl->consoleHandle, params[(size_t)color]);
	}
#else
	int params[] = 
	{
		31,32,34,
		33,36,35,
		37,37
	};
	if(color == Color::Gray)
		printf("\x1b[2;%dm", params[(size_t)color]); // Dim
	else
		printf("\x1b[1;%dm", params[(size_t)color]); // Bright
#endif
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

#ifdef _WIN32
String Utility::WindowsFormatMessage(uint32 code)
{
	if(code == 0)
	{
		return "No additional info available";
	}

	char buffer[1024] = {0};
	FormatMessageA(FORMAT_MESSAGE_FROM_SYSTEM, 0, code, LANG_SYSTEM_DEFAULT, buffer, sizeof(buffer), 0);
	return buffer;
}
#endif
