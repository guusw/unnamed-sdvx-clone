#include "stdafx.h"
#include "..\include\Tests\Tests.hpp"

int main()
{
	String moduleName = Path::ReplaceExtension(Path::GetModuleName(), "");

	Logf("Running tests for %s", Logger::Info, moduleName);

	try
	{
		
	}
}

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
