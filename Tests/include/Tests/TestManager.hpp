#pragma once
#include <Shared/Debug.hpp>
#include <Shared/Bindable.hpp>

// Test failure exception
class TestFailure
{
public:
	TestFailure(String expression) : expression(expression)
	{
	}

	String expression;
	Debug::StackTrace stackTrace;
};

class TestCategory
{
public:
	TestCategory& operator+=(class TestCategory* categoryToAdd);
	TestCategory& operator+=(class Test* testToAdd);

protected:
	Vector<TestCategory*> m_categories;
	Vector<Test*> m_rootTests;
};

class Test
{
public:
	template<typename T>
	Test& operator+=(T exec);

};

class TestManager : public TestCategory
{
	TestManager();
public:
	~TestManager();
	static TestManager& Get();
};