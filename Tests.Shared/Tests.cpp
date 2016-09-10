#include "Shared/Shared.hpp"
#include "Shared/Enum.hpp"
#include "Shared/Config.hpp"

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

	return 0;
}
