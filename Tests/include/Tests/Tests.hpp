#pragma once
#include <Shared/Debug.hpp>
#include <Shared/Macro.hpp>

#define 
#define Test(__expr) if(!(__expr)) throw TestFailure(STRINGIFY(__expr));

// Test failure exception
class TestFailure
{
public:
	TestFailure(String expression) : expression(expression)
	{
	}

	String expression;
	Debug::StackTrace stackTrace;
}

class Test
{
	Test& operator=(const
};

class TestManager
{
	TestManager();
public:
	~TestManager();
	static TestManager& Get();

private:
};