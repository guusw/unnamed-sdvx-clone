#include "stdafx.h"
#include <Data/Yaml.hpp>
#include <Data/Serializer.hpp>
#include <Data/ClassSerializer.hpp>
#include <Data/CollectionSerializer.hpp>
#include <Data/VirtualCollectionSerializer.hpp>
#include <Data/YamlStream.hpp>
using namespace Data;

Test("Data.Yaml")
{
	using namespace Yaml;
	Mapping a;
	a.Add("Test", new Scalar(100));

	Mapping* testObjOriginal = new Mapping();
	a.Add("TestObj", testObjOriginal);

	Node* testObjNode = a.Find("TestObj");
	TestEnsure(testObjNode);
	TestEnsure(testObjNode->IsMapping());
	TestEnsure(testObjNode == testObjOriginal);

	testObjNode->AsMapping().Add("Test1", new Scalar(2151));

	Node* numberNode = testObjOriginal->Find("Test1");
	TestEnsure(numberNode && numberNode->IsScalar());
	Scalar& numberScalar = numberNode->AsScalar();
	TestEnsure(numberScalar.IsNumber());
	TestEnsure(numberScalar.ToInt() == 2151);
	TestEnsure(numberScalar.ToFloat() == 2151.0f);

	// Test sequence
	{
		Set<int> foundInts;
		Sequence seq;
		seq.Add(123);
		seq.Add(456);
		for(auto i : seq)
		{
			TestEnsure(i->IsScalar());
			foundInts.Add(i->AsScalar().ToInt());
		}

		TestEnsure(foundInts.size() == 2);
		TestEnsure(foundInts.Contains(123));
		TestEnsure(foundInts.Contains(456));
	}

	// Test Map
	{
		Set<String> foundKeys;
		Set<int> foundInts;

		Mapping seq;
		seq.Add("A", new Scalar(123));
		seq.Add("B", new Scalar(456));
		for(auto p : seq)
		{
			TestEnsure(p.value->IsScalar());
			foundKeys.Add(p.key->ToString());
			foundInts.Add(p.value->AsScalar().ToInt());
		}

		TestEnsure(foundKeys.size() == 2);
		TestEnsure(foundKeys.Contains("A"));
		TestEnsure(foundKeys.Contains("B"));

		TestEnsure(foundInts.size() == 2);
		TestEnsure(foundInts.Contains(123));
		TestEnsure(foundInts.Contains(456));
	}
}

Test("Data.NativeTypeSerializers")
{
	// Check default type serializers
	TestEnsure(Serializer::Find(typeid(float).hash_code()) != nullptr);
	TestEnsure(Serializer::Find(typeid(int32).hash_code()) != nullptr);
	TestEnsure(Serializer::Find(typeid(String).hash_code()) != nullptr);
	TestEnsure(Serializer::Find(typeid(bool).hash_code()) != nullptr);
	TestEnsure(Serializer::Find<float>() != nullptr);
	TestEnsure(Serializer::Find<int32>() != nullptr);
	TestEnsure(Serializer::Find<String>() != nullptr);
	TestEnsure(Serializer::Find<bool>() != nullptr);
}

Test("Data.VectorSerializer")
{
	Vector<float> floatVec = { 1.0f, 2.0f, 10.0f };

	Node* node;
	VectorSerializer<float> floatVecSerializer;
	TestEnsure(floatVecSerializer.Serialize(&floatVec, node));

	Vector<float> replicated;
	TestEnsure(floatVecSerializer.Deserialize(*node, &replicated));

	TestEnsure(replicated.size() == floatVec.size());

	for(size_t i = 0; i < floatVec.size(); i++)
	{
		TestEnsure(floatVec[i] == replicated[i]);
	}
}

Test("Data.MapSerializer")
{
	Map<String, Color> colorMap;

	colorMap.Add("Red") = Color::Red;
	colorMap.Add("Green") = Color::Green;
	colorMap.Add("Blue") = Color::Blue;

	Node* node;
	MapSerializer<String, Color> serializer;
	TestEnsure(serializer.Serialize(&colorMap, node));

	decltype(colorMap) replicated;
	TestEnsure(serializer.Deserialize(*node, &replicated));

	TestEnsure(replicated.size() == colorMap.size());

	for(auto e : colorMap)
	{
		auto other = replicated.Find(e.first);
		TestEnsure(other);
		TestEnsure(*other == e.second);
	}
}

namespace
{
	class TestClass
	{
	public:
		float a;
		float b;
		int32 ia;
		int32 ib;
		Vector4 vectorVal;
		Color colorVal;
		String stringVal;
	};

	using namespace Data;
	using namespace Yaml;
	class TestClassSerializer : public ClassSerializer<TestClass>
	{
	public:
		TestClassSerializer()
		{
			RegisterClassMember(a);
			RegisterClassMember(b);
			RegisterClassMember(ia);
			RegisterClassMember(ib);
			RegisterClassMember(vectorVal);
			RegisterClassMember(colorVal);
			RegisterClassMember(stringVal);
		}
	};
	RegisterSerializer(TestClassSerializer);

	Test("Data.ClassSerializer")
	{
		ITypeSerializer* mySerializer = Serializer::Find<TestClass>();
		TestEnsure(mySerializer);

		TestClass a = { 1.0f, 2.0f, 10, 20 };
		a.vectorVal = Vector4(10, 4, 2, 1);
		a.colorVal = Color::Red.WithAlpha(0.4f);
		a.stringVal = "Lorem Ipsum";
		Node* nodeOut;
		mySerializer->Serialize(&a, nodeOut);

		TestClass b;
		mySerializer->Deserialize(*nodeOut, &b);

		TestEnsure(a.a == b.a);
		TestEnsure(a.b == b.b);
		TestEnsure(a.ia == b.ia);
		TestEnsure(a.ib == b.ib);
		TestEnsure(a.stringVal == b.stringVal);
		TestEnsure(memcmp(&a.vectorVal, &b.vectorVal, sizeof(float) * 4) == 0);
		TestEnsure(memcmp(&a.colorVal, &b.colorVal, sizeof(float) * 4) == 0);

		delete nodeOut;
	}

	class DerivedClass : public TestClass
	{
	public:
		int additionalMember;
	};
	class DerivedClassSerializer : public BaseClassSerializer<DerivedClass, TestClass>
	{
	public:
		DerivedClassSerializer()
		{
			RegisterClassMember(additionalMember);
		}
	};
	RegisterSerializer(DerivedClassSerializer);

	Test("Data.Serialization_ClassInheritance")
	{
		DerivedClass a;
		a.vectorVal = Vector4(10, 4, 2, 1);
		a.stringVal = "Lorem Ipsum";
		a.additionalMember = 10;

		ITypeSerializer* mySerializer = Serializer::Find<DerivedClass>();
		TestEnsure(mySerializer);

		Node* nodeOut;
		mySerializer->Serialize(&a, nodeOut);

		DerivedClass b;
		mySerializer->Deserialize(*nodeOut, &b);

		TestEnsure(a.additionalMember == b.additionalMember);
		TestEnsure(a.stringVal == b.stringVal);
		TestEnsure(memcmp(&a.vectorVal, &b.vectorVal, sizeof(float) * 4) == 0);

		delete nodeOut;
	}

	Test("Data.YamlReadTest")
	{
		Ref<Node> data;
		// Read a test file
		File file;


		TestEnsure(file.OpenRead("test\\test.yaml"));
		FileReader reader(file);
		TestEnsure(data = YamlStream::Read(reader));
	}
}

class TreeObject
{
public:
	TreeObject(const String& name = "<nameless TreeObject>") : name(name) {};
	virtual ~TreeObject() { for(auto& c : children) delete c; }
	String name;
	Vector<TreeObject*> children;
};
class SubTreeObject1 : public TreeObject
{
	using TreeObject::TreeObject;
	int testInt;
	float testFloat;
};
class SubTreeObject2 : public TreeObject
{
	using TreeObject::TreeObject;
	int testInt;
	float testFloat;
};

class TreeObjectSerializer : public ClassSerializer<TreeObject>
{
public:
	VirtualVectorSerializer<TreeObject> childSerializer;
	TreeObjectSerializer()
	{
		RegisterClassMember(name);
		childSerializer.RegisterType<TreeObject>("TreeObject");
		childSerializer.RegisterType<SubTreeObject1>("SubTreeObject1");
		childSerializer.RegisterType<SubTreeObject2>("SubTreeObject2");
	}
	virtual bool DeserializeUserProperties(Mapping& map, TreeObject& object) override
	{
		auto childrenNode = map.Find("Children");
		if(childrenNode)
		{
			if(!childrenNode->IsSequence())
				return false;
			if(!childSerializer.Deserialize(childrenNode->AsSequence(), object.children))
				return false;
		}
		return true;
	}
	virtual bool SerializeUserProperties(const TreeObject& object, Mapping& map) override
	{
		Sequence* children = new Sequence();
		map.Add("Children", children);
		if(!childSerializer.Serialize(object.children, *children))
			return false;
		return true;
	}
};
RegisterSerializer(TreeObjectSerializer);

struct A
{
	virtual ~A() = default;
	float a;
};
struct B : public A
{
	int b;
};
struct C : public A
{
	void* c;
};
struct ASerializer : public ClassSerializer<A>
{
	ASerializer() { RegisterClassMember(a); }
};
struct BSerializer : public BaseClassSerializer<B, A>
{
	BSerializer() { RegisterClassMember(b); }
};
struct CSerializer : public BaseClassSerializer<C, A>
{
	CSerializer() { RegisterClassMember(c); }
};
RegisterSerializer(ASerializer);
RegisterSerializer(BSerializer);
RegisterSerializer(CSerializer);

Test("Data.VirtualObjectSerializer")
{
	VirtualObjectSerializer<A> vobjSerializer;
	vobjSerializer.RegisterType<A>("A");
	vobjSerializer.RegisterType<B>("B");
	vobjSerializer.RegisterType<C>("C");

	A testObject;
	testObject.a = 100;

	Node* node;
	TestEnsure(vobjSerializer.Serialize(&testObject, node));

	A* replicated;
	TestEnsure(vobjSerializer.Deserialize(*node, &replicated));

	TestEnsure(replicated);
	TestEnsure(replicated->a == testObject.a);

	delete node;
	delete replicated;

	B testObject1;
	testObject1.a = 101.10f;
	testObject1.b = 20;

	TestEnsure(vobjSerializer.Serialize(&testObject1, node));

	TestEnsure(vobjSerializer.Deserialize(*node, &replicated));

	B* casted = dynamic_cast<B*>(replicated);
	TestEnsure(casted);
	TestEnsure(casted->b == testObject1.b);
	TestEnsure(casted->a == testObject1.a);
}

Test("Data.TreeTest")
{
	TreeObject testObject;
	testObject.children.Add(new TreeObject("A"));
	testObject.children.Add(new SubTreeObject1("B"));
	auto& c = testObject.children.Add(new SubTreeObject2("C"));
	c->children.Add(new TreeObject("D"));

	//Node* node;
	//TestEnsure(vobjSerializer.Serialize(&testObject, node));
	//
	//TreeObject* replicated;
	//TestEnsure(vobjSerializer.Deserialize(*node, replicated));
	//
	//TestEnsure(replicated);
	//TestEnsure(replicated->name == testObject.name);
	//
	//// Compare collections
	//auto& ca = testObject.children;
	//auto& cb = replicated->children;
	//TestEnsure(ca.size() == cb.size());
	//
	//for(size_t i = 0; i < ca.size(); i++)
	//{
	//	TestEnsure(typeid(ca[i]) == typeid(cb[i]));
	//}
	//TestEnsure(dynamic_cast<SubTreeObject2*>(cb[2]) != nullptr);
}