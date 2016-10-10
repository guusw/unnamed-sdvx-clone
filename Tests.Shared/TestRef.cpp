#include "stdafx.h"

struct A : public RefCounted<A>
{
	virtual ~A() = default;
	float a = 1.0f;
	float b = 2.0f;
};
struct C : public A
{
	String myString = "ok";
};
struct B
{
	int a = 3;
	int b = 2;
};

void FunctionTest(Ref<A> takingARef)
{

}

Test("Ref.RefInitializer")
{
	A* ap = new A();
	Ref<A> refa = *ap;
	Ref<A> refa1 = *ap;
	TestEnsure(refa.GetRefCount() == 2);

	FunctionTest(*ap);
	TestEnsure(refa.GetRefCount() == 2);

	Ref<A> c = *new C();
	Ref<C> c1 = c.As<C>();
	TestEnsure(c1.GetRefCount() == 2);
}