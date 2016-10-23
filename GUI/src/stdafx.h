#pragma once

#include <Shared/Shared.hpp>
#include <Graphics/Graphics.hpp>

// Asset loading macro
#define CheckedLoad(__stmt) if(!(__stmt)){Logf("Failed to load asset [%s]", Logger::Error, #__stmt); return false; }

// Common includes from this library
#include "Canvas.hpp"
#include "GUIEvents.hpp"
#include "GUIRenderer.hpp"