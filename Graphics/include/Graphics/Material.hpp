#pragma once
#include <Graphics/ResourceTypes.hpp>
#include <Graphics/Shader.hpp>
#include <Graphics/RenderState.hpp>

namespace Graphics
{
	/* A single parameter that is set for a material */
	struct MaterialParameter
	{
		CopyableBuffer parameterData;
		uint32 parameterType;

		template<typename T>
		static MaterialParameter Create(const T& obj, uint32 type)
		{
			MaterialParameter r;
			r.Bind(obj);
			r.parameterType = type;
			return r;
		}
		template<typename T>
		void Bind(const T& obj)
		{
			parameterData.resize(sizeof(T));
			memcpy(parameterData.data(), &obj, sizeof(T));
		}
		template<typename T>
		const T& Get()
		{
			assert(sizeof(T) == parameterData.size());
			return *(T*)parameterData.data();
		}
	};

	/*
		A list of parameters that is set for a material
		use SetParameter(name, param) to set any parameter by name
	*/
	class MaterialParameterSet : public Map<String, MaterialParameter>
	{
	public:
		using Map<String, MaterialParameter>::Map;
		void SetParameter(const String& name, float sc);
		void SetParameter(const String& name, const Vector4& vec);
		void SetParameter(const String& name, const Colori& color);
		void SetParameter(const String& name, const Vector2& vec2);
		void SetParameter(const String& name, const Vector2i& vec2);
		void SetParameter(const String& name, const Transform& tf);
		void SetParameter(const String& name, Ref<class TextureRes> tex);
	};

	enum class MaterialBlendMode
	{
		Normal,
		Additive,
		Multiply,
	};

	/*
		Abstracts the use of shaders/uniforms/pipelines into a single interface class
	*/
	class MaterialRes
	{
	public:
		virtual ~MaterialRes() = default;
		// Create a default material
		static Ref<MaterialRes> Create(class OpenGL* gl);
		// Create a material that has both a vertex and fragment shader
		static Ref<MaterialRes> Create(class OpenGL* gl, const String& vsPath, const String& fsPath);

		bool opaque = true;
		MaterialBlendMode blendMode = MaterialBlendMode::Normal;

	public:
		virtual void AssignShader(ShaderType t, Shader shader) = 0;
		virtual void Bind(const RenderState& rs, const MaterialParameterSet& params = MaterialParameterSet()) = 0;
	};

	typedef Ref<MaterialRes> Material;

	DEFINE_RESOURCE_TYPE(Material, MaterialRes);
}