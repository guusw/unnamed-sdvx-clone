#include "stdafx.h"
#include "MeshGenerators.hpp"

Mesh MeshGenerators::Quad(OpenGL* gl, Vector2 pos, Vector2 size /*= Vector2(1,1)*/)
{
	Vector<SimpleVertex> verts = 
	{
		{ { 0.0f,  size.y, 0.0f }, { 0.0f, 0.0f } },
		{ { size.x, 0.0f,  0.0f }, { 1.0f, 1.0f } },
		{ { size.x, size.y, 0.0f }, { 1.0f, 0.0f } },

		{ { 0.0f,  size.y, 0.0f }, { 0.0f, 0.0f } },
		{ { 0.0f,  0.0f,  0.0f }, { 0.0f, 1.0f } },
		{ { size.x, 0.0f,  0.0f }, { 1.0f, 1.0f } },
	};

	for(auto& v : verts)
	{
		v.pos += pos;
	}

	Mesh mesh = MeshRes::Create(gl);
	mesh->SetData(verts);
	mesh->SetPrimitiveType(PrimitiveType::TriangleList);
	return mesh;
}