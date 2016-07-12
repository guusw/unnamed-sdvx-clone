#include "stdafx.h"
#include "Application.hpp"

int32 __stdcall WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nShowCmd)
{
	new Application();
	int32 ret = g_application->Run();
	delete g_application;
	return 0;
}