/* Main and precompiled header file for Main project*/
#pragma once

// OpenGL Extension wrangler
#include "GL/glew.h"
#include "GL/wglew.h"

// Exclude rarely-used stuff from Windows headers
#define WIN32_LEAN_AND_MEAN
// Windows Header Files:
#include <windows.h>


// C RunTime Header Files
#include <stdlib.h>
#include <malloc.h>
#include <memory.h>
#include <tchar.h>
#include <cinttypes>

// TODO: reference additional headers your program requires here
#include <Shared/Shared.hpp>