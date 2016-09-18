#pragma once

class ProfilerScope
{
public:
	ProfilerScope(const String& name) : name(name)
	{
		Logf("Starting task \"%s\"", Logger::Info, name);
	}
	~ProfilerScope()
	{
		Logf("Finished task \"%s\" in  %d ms", Logger::Info, name, t.Milliseconds());
	}
private:
	Timer t;
	String name;
};