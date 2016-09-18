#include <Shared/Shared.hpp>
#include <Shared/Enum.hpp>
#include <Shared/Config.hpp>
#include <Shared/Debug.hpp>
#include <Shared/Macro.hpp>
#include <Tests/Tests.hpp>

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

Test("Test1")
{
	uint32* a = nullptr;
	a[10] = 5;
}
Test("Test2")
{
	int a = 2;
	int b = 3;
	TestEnsure(a + b == 5);
}
Test("Test3")
{
	int a = 2;
	int b = 5;
	TestEnsure(a - b == -1);
}
Test("Test4")
{
	int a = 2;
	int b = 5;
	Vector<int> v;
	v.Add(a);
	v.Add(b);
	v.Add();
	TestEnsure(*v.end() == 1);
}