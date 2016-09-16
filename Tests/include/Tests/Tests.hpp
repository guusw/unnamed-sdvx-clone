#pragma once
#include "TestManager.hpp"
#include <Shared/Macro.hpp>

#define TestCategory(catName)
#define Test(testName)
#define TestEnsure(__expr) if(!(__expr)) throw TestFailure(STRINGIFY(__expr));