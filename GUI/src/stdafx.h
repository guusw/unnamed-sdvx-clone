#pragma once

#include <Shared/Shared.hpp>
#include <Graphics/Graphics.hpp>
using namespace Graphics;

// Asset loading macro
#define CheckedLoad(__stmt) if(!(__stmt)){Logf("Failed to load asset [%s]", Logger::Error, #__stmt); return false; }