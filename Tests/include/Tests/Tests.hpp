#pragma once
#include "TestManager.hpp"
#include <Shared/Macro.hpp>

#define Test(testName)\
static void CONCAT(LocalTest, __LINE__)();\
static TestEntry* CONCAT(te, __LINE__) = new TestEntry(testName, &CONCAT(LocalTest, __LINE__));\
void CONCAT(LocalTest, __LINE__)()
#define TestEnsure(__expr) if(!(__expr)) throw TestFailure(STRINGIFY(__expr));