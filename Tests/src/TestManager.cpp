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

	_crt_signal_t oldSigHandler = signal(SIGSEGV, SignalHandler);
	String moduleName = Path::ReplaceExtension(Path::GetModuleName(), "");
	Logf("Running tests for %s", Logger::Info, moduleName);

	int32 failed = 0;
	for(int32 i = 0; i < m_tests.size(); i++)
	{
		Logger::Get().SetColor(Logger::White);
		Logger::Get().WriteHeader(Logger::Info);
		Logger::Get().Write(Utility::Sprintf("Running test [%s]: ", m_tests[i]->m_name));

		try
		{
			m_tests[i]->m_function();
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

		Logger::Get().SetColor(Logger::Green);
		Logger::Get().Write("Passed\n");
	}

	signal(SIGSEGV, oldSigHandler);
	return failed;
}

TestEntry::TestEntry(String name, TestFunction function) : m_name(name), m_function(function)
{
	TestManager::Get().m_tests.Add(this);
}
