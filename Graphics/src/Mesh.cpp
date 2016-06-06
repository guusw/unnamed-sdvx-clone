#include "stdafx.h"
#include "Mesh.hpp"
#include <Graphics/ResourceManager.hpp>

uint32 primitiveTypeMap[] =
{
	GL_TRIANGLES,
	GL_TRIANGLE_STRIP,
	GL_TRIANGLE_FAN,
	GL_LINES,
	GL_LINE_STRIP,
	GL_POINTS,
};
class Mesh_Impl : public MeshRes
{
	uint32 m_buffer = 0;
	uint32 m_vao = 0;
	PrimitiveType m_type;
	uint32 m_glType;
	size_t m_vertexCount;
	bool m_bDynamic = true;
public:
	Mesh_Impl()
	{
	}
	void Dispose()
	{
		if (m_buffer)
			glDeleteBuffers(1, &m_buffer);
		if (m_vao)
			glDeleteVertexArrays(1, &m_vao);
	}
	bool Init()
	{
		glGenBuffers(1, &m_buffer);
		glGenVertexArrays(1, &m_vao);
		return m_buffer != 0 && m_vao != 0;
	}

	virtual void SetData(const void* pData, size_t vertexCount, const VertexFormatList& desc)
	{
		glBindVertexArray(m_vao);
		glBindBuffer(GL_ARRAY_BUFFER, m_buffer);

		m_vertexCount = vertexCount;
		size_t totalVertexSize = 0;
		for (auto e : desc)
			totalVertexSize += e.componentSize * e.components;
		size_t index = 0;
		size_t offset = 0;
		for (auto e : desc)
		{
			uint32 type = -1;
			if(!e.isFloat)
			{
				if(e.componentSize == 4)
					type = e.isSigned ? GL_INT : GL_UNSIGNED_INT;
				else if(e.componentSize == 2)
					type = e.isSigned ? GL_SHORT : GL_UNSIGNED_SHORT;
				else if(e.componentSize == 1)
					type = e.isSigned ? GL_BYTE : GL_UNSIGNED_BYTE;
			}
			else
			{
				if(e.componentSize == 4)
					type = GL_FLOAT;
				else if(e.componentSize == 8)
					type = GL_DOUBLE;
			}
			assert(type != -1);
			glVertexAttribPointer((int)index, (int)e.components, type, GL_TRUE, (int)totalVertexSize, (void*)offset);
			glEnableVertexAttribArray((int)index);
			offset += e.componentSize * e.components;
			index++;
		}
		glBufferData(GL_ARRAY_BUFFER, totalVertexSize * vertexCount, pData, m_bDynamic ? GL_DYNAMIC_DRAW : GL_STATIC_DRAW);

		glBindVertexArray(0);
		glBindBuffer(GL_ARRAY_BUFFER, 0);
	}
	virtual void Draw()
	{
		glBindVertexArray(m_vao);
		glDrawArrays(m_glType, 0, (int)m_vertexCount);
	}

	virtual void SetPrimitiveType(PrimitiveType pt)
	{
		m_type = pt;
		m_glType = primitiveTypeMap[(size_t)pt];
	}
	virtual PrimitiveType GetPrimitiveType() const override
	{
		return m_type;
	}
};

Mesh MeshRes::Create(class OpenGL* gl)
{
	Mesh_Impl* pImpl = new Mesh_Impl();
	if (!pImpl->Init())
	{
		delete pImpl;
		return Mesh();
	}
	else
	{
		return GetResourceManager<ResourceType::Mesh>().Register(pImpl);
	}
}