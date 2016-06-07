/*
	OpenGL include file
	This file includes the appropriate opengl headers for the platform
*/
#pragma once

// Link statically against glew
#ifndef GLEW_STATIC
#define GLEW_STATIC
#endif

// Don't include GLU in glew.h
#define GLEW_NO_GLU

#include <Graphics/glew/GL/glew.h>

// Include platform specific OpenGL headers
#ifdef _WIN32
#include <Graphics/glew/GL/wglew.h>
#else
#include <Graphics/glew/GL/glxew.h>
#endif

