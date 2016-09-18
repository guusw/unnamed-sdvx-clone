#include "stdafx.h"
#include "TestManager.hpp"
#include <signal.h>

TestManager::TestManager()
{
}
TestManager::~TestManager()
{
}
TestManager& TestManager::Get()
{
	static TestManager inst;
	return inst;
}

int32 TestManager::RunTests()
{
	auto SignalHandler = [](int sig)->void
	{
		throw TestFailure();
	};

	void (*oldSigHandler)(int) = 0;
	if(!Debug::IsDebuggerAttached())
	{
		oldSigHandler = signal(SIGSEGV, SignalHandler);
	}

	String moduleName = Path::GetModuleName();
	Logf("Running tests for %s", Logger::Info, moduleName);

	// Create intermediate folder
	m_testBasePath = Path::Absolute("TestFilesystem_" + moduleName);
	if(Path::FileExists(m_testBasePath))
		Path::DeleteDir(m_testBasePath);
	if(!Path::CreateDir(m_testBasePath))
	{
		Logf("Failed to create folder for intermediate test files: %s", Logger::Info, m_testBasePath);
		return -1;
	}

	int32 failed = 0;
	for(int32 i = 0; i < m_tests.size(); i++)
	{
		Logger::Get().SetColor(Logger::White);
		Logger::Get().WriteHeader(Logger::Info);
		Logger::Get().Write(Utility::Sprintf("Running test [%s]: ", m_tests[i]->m_name));

		TestContext context(m_tests[i]->m_name, this);

		if(Debug::IsDebuggerAttached())
		{
			// Don't catch any exceptions while debugging
			m_tests[i]->m_function(context);
		}
		else
		{
			try
			{
				m_tests[i]->m_function(context);
			}
			catch(TestFailure tf)
			{

				Logger::Get().SetColor(Logger::Red);
				Logger::Get().Write("Failed\n");
				if(!tf.expression.empty())
					Logf("The test expression failed:\n\t%s", Logger::Error, tf.expression);
				Logf("Stack Trace:", Logger::Error);

				size_t i = 0;
				for(; i < tf.trace.size(); i++)
				{
					// Skip until wanted function
					if(tf.trace[i].function.find("LocalTest") == 0)
						break;
				}
				if(i == tf.trace.size())
					i = 0; // Show everything if wanted symbol was not found

				for(; i < tf.trace.size(); i++)
				{
					Debug::StackFrame sf = tf.trace[i];
					Logf("%016X %s (%d:%s)", Logger::Error, sf.address, sf.function, sf.line, sf.file);
				}
				failed++;
				continue;
			}
		}

		Logger::Get().SetColor(Logger::Green);
		Logger::Get().Write("Passed\n");
	}

	// Remove intermediate folder
	Path::DeleteDir(m_testBasePath);

	// Restore signal handler
	if(!Debug::IsDebuggerAttached())
		signal(SIGSEGV, oldSigHandler);

	return failed;
}

TestEntry::TestEntry(String name, TestFunction function) : m_name(name), m_function(function)
{
	TestManager::Get().m_tests.Add(this);
}
String TestContext::GenerateTestFilePath() const
{
	return m_testManager->m_testBasePath + Path::sep + "Test_" + m_name;
}
String TestContext::GetTestBasePath() const
{
	return m_testManager->m_testBasePath;
}
