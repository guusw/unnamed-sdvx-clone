#include "stdafx.h"
#include <Data/PropertyHierarchy.hpp>
#include <Data/Serializer.hpp>
using namespace Data;

Test("Data.PropertyHierarchy")
{
	Object a("Root");
	a.Set("MyList", Data::List("Test"));
	Data::List* list = dynamic_cast<Data::List*>(a.Get("MyList"));
	TestEnsure(list != nullptr);
	list->Add(Object("1"));
	list->Add(Object("2"));
	TestEnsure(list->GetSize() == 2);
}

Test("Data.Serializer")
{

}