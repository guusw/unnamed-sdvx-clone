/*
	OpenGL include file
	This file includes the appropriate opengl headers for the platform
*/
#pragma once

// Link statically against glew
#ifndef GLEW_STATIC
#define GLEW_STATIC
#endif

#include <graphics/glew/GL/glew.h>

// Include platform specific OpenGL headers
#ifdef _WIN32
#include <graphics/glew/GL/wglew.h>
#else
#include <graphics/glew/GL/glxew.h>
#endif

