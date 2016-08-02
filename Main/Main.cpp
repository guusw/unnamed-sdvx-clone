#include "stdafx.h"
#include "Application.hpp"

#ifdef _WIN32
int32 __stdcall WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nShowCmd)
#else
int main(int argc, char **argv)
#endif
{
	new Application();
	int32 ret = g_application->Run();
	delete g_application;
	return ret;
}