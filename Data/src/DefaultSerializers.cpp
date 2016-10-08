#include "stdafx.h"
#include "ClassSerializer.hpp"

namespace Data
{
	using namespace Yaml;

	class IntSerializer : public ITypeSerializer
	{
	public:
		virtual bool Deserialize(Node& data, void* object) override
		{
			if(!data.IsScalar())
				return false;
			*(int32*)object = data.AsScalar().ToInt();
			return true;
		}
		virtual bool Serialize(const void* object, Node*& data) override
		{
			data = new Scalar(*(int32*)object);
			return true;
		}
	};

	class FloatSerializer : public ITypeSerializer
	{
	public:
		virtual bool Deserialize(Node& data, void* object) override
		{
			if(!data.IsScalar())
				return false;
			*(float*)object = data.AsScalar().ToFloat();
			return true;
		}
		virtual bool Serialize(const void* object, Node*& data) override
		{
			data = new Scalar(*(float*)object);
			return true;
		}
	};

	class StringSerializer : public ITypeSerializer
	{
	public:
		virtual bool Deserialize(Node& data, void* object) override
		{
			if(!data.IsScalar())
				return false;
			*(String*)object = data.AsScalar().ToString();
			return true;
		}
		virtual bool Serialize(const void* object, Node*& data) override
		{
			data = new Scalar(*(String*)object);
			return true;
		}
	};

	class BoolSerialzier : public ITypeSerializer
	{
	public:
		virtual bool Deserialize(Node& data, void* object) override
		{
			if(!data.IsScalar())
				return false;
			*(bool*)object = data.AsScalar().ToBoolean();
			return true;
		}
		virtual bool Serialize(const void* object, Node*& data) override
		{
			data = new Scalar(*(bool*)object);
			return true;
		}
	};

	class Vector2Serializer : public ClassSerializer<Vector2>
	{
	public:
		Vector2Serializer()
		{
			RegisterClassMember(x);
			RegisterClassMember(y);
		}
	};
	class Vector3Serializer : public ClassSerializer<Vector3>
	{
	public:
		Vector3Serializer()
		{
			RegisterClassMember(x);
			RegisterClassMember(y);
			RegisterClassMember(z);
		}
	};
	class Vector4Serializer : public ClassSerializer<Vector4>
	{
	public:
		Vector4Serializer()
		{
			RegisterClassMember(x);
			RegisterClassMember(y);
			RegisterClassMember(z);
			RegisterClassMember(w);
		}
	};
	class ColorSerializer : public ClassSerializer<Color>
	{
	public:
		ColorSerializer()
		{
			RegisterClassMemberNamed("r", x);
			RegisterClassMemberNamed("g", y);
			RegisterClassMemberNamed("b", z);
			RegisterClassMemberNamed("a", w);
		}
	};

	void RegisterDefaultSerializers()
	{
		Serializer::Register<float>(new FloatSerializer());
		Serializer::Register<int32>(new IntSerializer());
		Serializer::Register<bool>(new BoolSerialzier());
		Serializer::Register<String>(new StringSerializer());

		Serializer::Register<Vector2>(new Vector2Serializer());
		Serializer::Register<Vector3>(new Vector3Serializer());
		Serializer::Register<Vector4>(new Vector4Serializer());
		Serializer::Register<Color>(new ColorSerializer());
	}
}