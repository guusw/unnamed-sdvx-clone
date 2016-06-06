/* Main and precompiled header file for Main project*/
#pragma once

// OpenGL headers
#include <Graphics/GL.hpp>

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

// Graphics components
#include <Graphics/OpenGL.hpp>
#include <Graphics/Image.hpp>
#include <Graphics/ImageLoader.hpp>
#include <Graphics/Texture.hpp>
#include <Graphics/Material.hpp>
#include <Graphics/Mesh.hpp>
#include <Graphics/RenderQueue.hpp>
#include <Graphics/RenderState.hpp>
#include <Graphics/ParticleSystem.hpp>
#include <Graphics/MeshGenerators.hpp>
#include <Graphics/Font.hpp>
#include <Graphics/Framebuffer.hpp>

// Asset loading macro
#define CheckedLoad(__stmt) if(!(__stmt)){Logf("Failed to load asset [%s]", Logger::Error, #__stmt); return false; }