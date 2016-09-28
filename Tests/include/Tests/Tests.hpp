#pragma once
#include "TestManager.hpp"
#include <Shared/Macro.hpp>

// Macro used to define a test to be performed
// use TestEnsure inside the macro to check for boolean conditions
#define Test(testName)\
static void CONCAT(LocalTest, __LINE__)(TestContext& context);\
static TestEntry* CONCAT(te, __LINE__) = new TestEntry(testName, &CONCAT(LocalTest, __LINE__));\
void CONCAT(LocalTest, __LINE__)(TestContext& context)

// Test assertion macro
#define TestEnsure(__expr) if(!(__expr)) throw TestFailure(STRINGIFY(__expr));

// A single filename assigned to be used by this test as a temporary file
#define TestFilename context.GenerateTestFilePath()
// A file path that is used as the location for test-local files
#define TestBasePath context.GetTestBasePath()

// Main entry point for running tests
int TestMain();