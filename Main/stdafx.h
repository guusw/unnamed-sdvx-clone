/* Main and precompiled header file for Main project*/
#pragma once

// OpenGL Extension wrangler
#include "GL/glew.h"
#include "GL/wglew.h"

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

// Asset loading macro
#define CheckedLoad(__stmt) if(!(__stmt)){Logf("Failed to load asset [%s]", Logger::Error, #__stmt); return false; }