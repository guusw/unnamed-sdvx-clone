#include "Shared/Shared.hpp"
#include "Shared/Enum.hpp"

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


int main(void)
{	
	test(Enum_TestEnum::ToString(TestEnum::OO) == "OO");

	return 0;
}