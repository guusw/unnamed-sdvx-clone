#include "Shared/Shared.hpp"
#include "Shared/Enum.hpp"
#include "Shared/Config.hpp"
#include "Shared/Debug.hpp"
#include <zlib.h>

#define _STRINGIFY(__x) #__x
#define STRINGIFY(__x) _STRINGIFY(__x)
#define test(__exp) if(!(__exp)){\
	Logf("Test failed (%s)", Logger::Error, STRINGIFY(__exp));\
	throw int();\
}

DefineEnum(TestEnum,
	A = 0x0,
	B = 0x2,
	C,
	D,
	OO);

DefineBitflagEnum(TestFlags,
    Color = 0x1,
    Float = 0x2,
    Transparent = 0x4);
ImplementBitflagEnum(TestFlags);

class MyConfig : public Config<Enum_TestEnum>
{
public:
    MyConfig()
    {
        // Defualts
        Clear();
    }
protected:
    virtual void InitDefaults() override
    {
        Set(TestEnum::A, 10);
        Set(TestEnum::B, 20.0f);
        SetEnum<Enum_TestFlags>(TestEnum::C, TestFlags::Float | TestFlags::Transparent);
    }
};

int main(void)
{
	Logf("Started", Logger::Info);
	test(Enum_TestEnum::ToString(TestEnum::OO) == "OO");

	MyConfig c;
	c.Save("file");

	try
	{
		Vector<float> floats;
		floats.resize(10);
		(*floats.end()) = 20.0f;
		uint32* test = 0;
		test[0] = 20;
	}
	catch(...)
	{
		auto stackTrace = Debug::GetStackTrace();
		Logf("Program crashed, dumping stack trace", Logger::Error);
		for(uint32 i = 0; i < stackTrace.size(); i++)
		{
			auto& sf = stackTrace[i];
			Logf("[%d: 0x%016X] %s@%d %s", Logger::Warning, i, sf.address, sf.file, sf.line, sf.function);
		}
		return 1;
	}
	return 0;
}
