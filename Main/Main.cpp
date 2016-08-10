#include "stdafx.h"
#include "Application.hpp"
#include <string>

#ifdef _WIN32
int32 __stdcall WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nShowCmd)
#else
int main(int argc, char **argv)
#endif
{
    std::string command;
    for(int i=0; i<argc; i++) {
        command += argv[i];
        command += " ";
    }
    new Application(command);
	int32 ret = g_application->Run();
	delete g_application;
	return 0;
}
