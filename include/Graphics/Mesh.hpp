#pragma once
#include <Graphics/ResourceTypes.hpp>
#include <Graphics/VertexFormat.hpp>

enum class PrimitiveType
{
	TriangleList = 0,
	TriangleStrip,
	TriangleFan,
	LineList,
	LineStrip,
	PointList,
};

class MeshRes
{
public:
	virtual ~MeshRes() = default;
	static Ref<MeshRes> Create(class OpenGL* gl);
public:
	template<typename T>
	void SetData(const Vector<T>& verts)
	{
		SetData(verts.data(), verts.size(), T::GetDescriptors());
	}
	virtual void SetPrimitiveType(PrimitiveType pt) = 0;
	virtual void Draw() = 0;
	virtual void BeginBatch() = 0;
	virtual void BatchDraw(uint32 offset, uint32 count) = 0;
private:
	virtual void SetData(const void* pData, size_t vertexCount, const VertexFormatList& desc) = 0;
};

typedef Ref<MeshRes> Mesh;

DEFINE_RESOURCE_TYPE(Mesh, MeshRes);