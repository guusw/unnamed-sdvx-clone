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
	typedef void(*TestFunction)(class TestContext& context);

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
	String m_testBasePath;
	friend class TestEntry;
	friend class TestContext;
};

class TestContext
{
public:
	TestContext(String name, TestManager* mgr) : m_name(name), m_testManager(mgr) {};
	String GenerateTestFilePath() const;
	String GetTestBasePath() const;
	String GetName() const { return m_name; }

private:
	TestManager* m_testManager;
	String m_name;
};