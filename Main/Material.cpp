#include "stdafx.h"
#include "Material.hpp"
#include "OpenGL.hpp"
#include "ResourceManager.hpp"
#include "RenderQueue.hpp"

// Defines build in shader variables
enum BuiltInShaderVariable
{
	SV_World = 0,
	SV_Proj,
	SV_Camera,
	SV_Viewport,
	SV_AspectRatio,
	SV__BuiltInEnd,
	SV_User = 0x100, // Start defining user variables here
};
const char* builtInShaderVariableNames[] =
{
	"world",
	"proj",
	"camera",
	"viewport",
	"aspectRatio",
};
class BuiltInShaderVariableMap : public Map<String, BuiltInShaderVariable>
{
public:
	BuiltInShaderVariableMap()
	{
		for(int32 i = 0; i < SV__BuiltInEnd; i++)
		{
			Add(builtInShaderVariableNames[i], (BuiltInShaderVariable)i);
		}
	}
};
BuiltInShaderVariableMap builtInShaderVariableMap;

struct BoundParameterInfo
{
	BoundParameterInfo(ShaderType shaderType, uint32 paramType, uint32 location)
		:shaderType(shaderType), paramType(paramType), location(location)
	{
	}

	ShaderType shaderType;
	uint32 location;
	uint32 paramType;
};
struct BoundParameterList : public Vector<BoundParameterInfo>
{
};

// Defined in Shader.cpp
extern uint32 shaderStageMap[];

class Material_Impl : public MaterialRes
{
public:
	OpenGL* m_gl;
	Shader m_shaders[3];
	uint32 m_pipeline;
	Map<uint32, BoundParameterList> m_boundParameters;
	Map<String, uint32> m_mappedParameters;
	Map<String, uint32> m_textureIDs;
	uint32 m_userID = SV_User;
	uint32 m_textureID = 0;

	Material_Impl(OpenGL* gl) : m_gl(gl)
	{
		glGenProgramPipelines(1, &m_pipeline);
	}
	~Material_Impl()
	{
		glDeleteProgramPipelines(1, &m_pipeline);
	}
	void AssignShader(ShaderType t, Shader shader)
	{
		m_shaders[(size_t)t] = shader;

		uint32 handle = shader->Handle();

		Logf("Listing shader uniforms for %s", Logger::Info, shader->GetOriginalName());
		int32 numUniforms;
		glGetProgramiv(handle, GL_ACTIVE_UNIFORMS, &numUniforms);
		for(int32 i = 0; i < numUniforms; i++)
		{
			char name[64];
			int32 nameLen, size;
			uint32 type;
			glGetActiveUniform(handle, i, sizeof(name), &nameLen, &size, &type, name);
			uint32 loc = glGetUniformLocation(handle, name);

			// Select type
			uint32 textureID = 0;
			String typeName = "Unknown";
			if(type == GL_SAMPLER_2D)
			{
				typeName = "Sampler2D";
				if(!m_textureIDs.Contains(name))
					m_textureIDs.Add(name, m_textureID++);
			}
			else if(type == GL_FLOAT_MAT4)
			{
				typeName = "Transform";
			}
			else if(type == GL_FLOAT_VEC4)
			{
				typeName = "Vector4";
			}
			else if(type == GL_FLOAT_VEC3)
			{
				typeName = "Vector3";
			}
			else if(type == GL_FLOAT_VEC2)
			{
				typeName = "Vector2";
			}
			else if(type == GL_FLOAT)
			{
				typeName = "Float";
			}

			// Built in variable?
			uint32 targetID = 0;
			if(builtInShaderVariableMap.Contains(name))
			{
				targetID = builtInShaderVariableMap[name];
			}
			else
			{
				if(m_mappedParameters.Contains(name))
					targetID = m_mappedParameters[name];
				else
					targetID = m_mappedParameters.Add(name, m_userID++);
			}

			BoundParameterInfo& param = m_boundParameters.FindOrAdd(targetID).Add(BoundParameterInfo(t, type, loc));

			Logf("Uniform [%d, loc=%d, %s] = %s", Logger::Info, 
				i, loc, Utility::Sprintf("Unknown [%d]", type), name);
		}

		glUseProgramStages(m_pipeline, shaderStageMap[(size_t)t], shader->Handle());
	}

	virtual void Bind(RenderState& rs, const MaterialParameterSet& params) override
	{
#if _DEBUG
		bool reloadedShaders = false;
		for(uint32 i = 0; i < 3; i++)
		{
			if(m_shaders[i] && m_shaders[i]->UpdateHotReload())
			{
				reloadedShaders = true;
			}
		}

		// Regenerate parameter map
		if(reloadedShaders)
		{
			Log("Reloading material", Logger::Info);
			m_boundParameters.clear();
			m_textureIDs.clear();
			m_mappedParameters.clear();
			m_userID = SV_User;
			m_textureID = 0;
			for(uint32 i = 0; i < 3; i++)
			{
				if(m_shaders[i])
					AssignShader(ShaderType(i), m_shaders[i]);
			}
		}
#endif

		// Bind renderstate variables
		BindAll(SV_World, rs.worldTransform);
		BindAll(SV_Proj, rs.projectionTransform);
		BindAll(SV_Camera, rs.cameraTransform);
		BindAll(SV_Viewport, rs.viewportSize);
		BindAll(SV_AspectRatio, rs.aspectRatio);
		for(auto p : params)
		{
			switch(p.second.parameterType)
			{
			case GL_FLOAT:
				BindAll(p.first, p.second.Get<float>());
				break;
			case GL_INT_VEC2:
				BindAll(p.first, p.second.Get<Vector2i>());
				break;
			case GL_INT_VEC3:
				BindAll(p.first, p.second.Get<Vector3i>());
				break;
			case GL_INT_VEC4:
				BindAll(p.first, p.second.Get<Vector4i>());
				break;
			case GL_FLOAT_VEC2:
				BindAll(p.first, p.second.Get<Vector2>());
				break;
			case GL_FLOAT_VEC3:
				BindAll(p.first, p.second.Get<Vector3>());
				break;
			case GL_FLOAT_VEC4:
				BindAll(p.first, p.second.Get<Vector4>());
				break;
			case GL_FLOAT_MAT4:
				BindAll(p.first, p.second.Get<Transform>());
				break;
			case GL_SAMPLER_2D:
			{				
				uint32* textureUnit = m_textureIDs.Find(p.first);
				if(!textureUnit)
				{
					/// TODO: Add print once mechanism for these kind of errors
					//Logf("Texture not found \"%s\"", Logger::Warning, p.first);
					break;
				}
				uint32 texture = p.second.Get<int32>();

				// Bind the texture
				glActiveTexture(GL_TEXTURE0 + *textureUnit);
				glBindTexture(GL_TEXTURE_2D, texture);

				// Bind sampler
				BindAll<int32>(p.first, *textureUnit);
				break;
			}
			default:
				assert(false);
			}
		}
		glBindProgramPipeline(m_pipeline);
	}

	BoundParameterInfo* GetBoundParameters(const String& name, uint32& count)
	{
		uint32* mappedID = m_mappedParameters.Find(name);
		if(!mappedID)
			return nullptr;
		return GetBoundParameters((BuiltInShaderVariable)*mappedID, count);
	}
	BoundParameterInfo* GetBoundParameters(BuiltInShaderVariable bsv, uint32& count)
	{
		BoundParameterList* l = m_boundParameters.Find(bsv);
		if(!l)
			return nullptr;
		else
		{
			count = (uint32)l->size();
			return l->data();
		}
	}
	template<typename T> void BindAll(const String& name, const T& obj)
	{
		uint32 num = 0;
		BoundParameterInfo* bp = GetBoundParameters(name, num);
		for(uint32 i = 0; bp && i < num; i++)
		{
			BindShaderVar<T>(m_shaders[(size_t)bp[i].shaderType]->Handle(), bp[i].location, obj);
		}
	}
	template<typename T> void BindAll(BuiltInShaderVariable bsv, const T& obj)
	{
		uint32 num = 0;
		BoundParameterInfo* bp = GetBoundParameters(bsv, num);
		for(uint32 i = 0; bp && i < num; i++)
		{
			BindShaderVar<T>(m_shaders[(size_t)bp[i].shaderType]->Handle(), bp[i].location, obj);
		}
	}

	template<typename T> void BindShaderVar(uint32 shader, uint32 loc, const T& obj)
	{
		static_assert(false, "Incompatible shader uniform type");
	}
	template<> void BindShaderVar<Vector4>(uint32 shader, uint32 loc, const Vector4& obj)
	{
		glProgramUniform4fv(shader, loc, 1, &obj.x);
	}
	template<> void BindShaderVar<Vector3>(uint32 shader, uint32 loc, const Vector3& obj)
	{
		glProgramUniform3fv(shader, loc, 1, &obj.x);
	}
	template<> void BindShaderVar<Vector2>(uint32 shader, uint32 loc, const Vector2& obj)
	{
		glProgramUniform2fv(shader, loc, 1, &obj.x);
	}
	template<> void BindShaderVar<float>(uint32 shader, uint32 loc, const float& obj)
	{
		glProgramUniform1fv(shader, loc, 1, &obj);
	}
	template<> void BindShaderVar<Colori>(uint32 shader, uint32 loc, const Colori& obj)
	{
		Color c = obj;
		glProgramUniform4fv(shader, loc, 1, &c.x);
	}
	template<> void BindShaderVar<Vector4i>(uint32 shader, uint32 loc, const Vector4i& obj)
	{
		glProgramUniform4iv(shader, loc, 1, &obj.x);
	}
	template<> void BindShaderVar<Vector3i>(uint32 shader, uint32 loc, const Vector3i& obj)
	{
		glProgramUniform3iv(shader, loc, 1, &obj.x);
	}
	template<> void BindShaderVar<Vector2i>(uint32 shader, uint32 loc, const Vector2i& obj)
	{
		glProgramUniform2iv(shader, loc, 1, &obj.x);
	}
	template<> void BindShaderVar<int32>(uint32 shader, uint32 loc, const int32& obj)
	{
		glProgramUniform1iv(shader, loc, 1, &obj);
	}
	template<> void BindShaderVar<Transform>(uint32 shader, uint32 loc, const Transform& obj)
	{
		glProgramUniformMatrix4fv(shader, loc, 1, GL_FALSE, obj.mat);
	}
};

Material MaterialRes::Create(OpenGL* gl)
{
	Material_Impl* impl = new Material_Impl(gl);
	return GetResourceManager<ResourceType::Material>().Register(impl);

}
Material MaterialRes::Create(OpenGL* gl, const String& vsPath, const String& fsPath)
{
	Material_Impl* impl = new Material_Impl(gl);
	impl->AssignShader(ShaderType::Vertex, ShaderRes::Create(gl, ShaderType::Vertex, vsPath));
	impl->AssignShader(ShaderType::Fragment, ShaderRes::Create(gl, ShaderType::Fragment, fsPath));

	if(!impl->m_shaders[(size_t)ShaderType::Vertex])
	{
		Logf("Failed to load vertex shader for material from %s", Logger::Error, vsPath);
		delete impl;
		return Material();
	}
	if(!impl->m_shaders[(size_t)ShaderType::Fragment])
	{
		Logf("Failed to load fragment shader for material from %s", Logger::Error, fsPath);
		delete impl;
		return Material();
	}

	return GetResourceManager<ResourceType::Material>().Register(impl);
}

void MaterialParameterSet::SetParameter(const String& name, float sc)
{
	Add(name, MaterialParameter::Create(sc, GL_FLOAT));
}
void MaterialParameterSet::SetParameter(const String& name, const Vector4& vec)
{
	Add(name, MaterialParameter::Create(vec, GL_FLOAT_VEC4));
}
void MaterialParameterSet::SetParameter(const String& name, const Colori& color)
{
	Add(name, MaterialParameter::Create(Color(color), GL_FLOAT_VEC4));
}
void MaterialParameterSet::SetParameter(const String& name, const Vector2& vec2)
{
	Add(name, MaterialParameter::Create(vec2, GL_FLOAT_VEC2));
}
void MaterialParameterSet::SetParameter(const String& name, const Transform& tf)
{
	Add(name, MaterialParameter::Create(tf, GL_FLOAT_MAT4));
}
void MaterialParameterSet::SetParameter(const String& name, Ref<class TextureRes> tex)
{
	Add(name, MaterialParameter::Create(tex->Handle(), GL_SAMPLER_2D));
}
