#include "stdafx.h"
#include <Shared/PropertyHierarchy.hpp>
using namespace PropertyHierarchy;

Test("PropertyHierarchy")
{
	Object a("Root");
	a.Set("MyList", PropertyHierarchy::List("Test"));
	PropertyHierarchy::List* list = dynamic_cast<PropertyHierarchy::List*>(a.Get("MyList"));
	TestEnsure(list != nullptr);
	list->Add(Object("1"));
	list->Add(Object("2"));
	TestEnsure(list->GetSize() == 2);
}