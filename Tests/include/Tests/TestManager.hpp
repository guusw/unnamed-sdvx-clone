#pragma once
#include <Shared/Debug.hpp>
#include <Shared/Bindable.hpp>

// Test failure exception
class TestFailure
{
public:
	TestFailure(String expression = String()) : expression(expression)
	{
		trace = Debug::GetStackTrace();
	}
	String expression;
	Debug::StackTrace trace;
};

class TestEntry
{
public:
	typedef void(*TestFunction)();

	TestEntry(String name, TestFunction function);

private:
	String m_name;
	TestFunction m_function;
	friend class TestManager;
};

class TestManager
{
	TestManager();
public:
	~TestManager();
	static TestManager& Get();
	int32 RunTests();

private:
	Vector<TestEntry*> m_tests;
	friend class TestEntry;
};