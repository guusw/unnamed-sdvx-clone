#pragma once
#include <Graphics/Mesh.hpp>
#include <Graphics/Texture.hpp>
#include <Graphics/Font.hpp>
#include <Graphics/Material.hpp>

namespace Graphics
{
	/*
		Rendering state which maintains and changes the state of the rendering context and avoid unnecessary changes
	*/
	class RenderQueueState
	{
	public:
		RenderQueueState(RenderState& rs, OpenGL* gl);
		~RenderQueueState();
		void SetMaterial(Material& mat, MaterialParameterSet& params);
		void DrawOrRedrawMesh(Mesh& mesh);
		void SetWorldTransform(Transform& transform);

		void SetScissorEnabled(bool enabled);
		void SetScissorRect(const Rect& rect);

		void Reset();

		RenderState& GetRenderState();
		OpenGL& GetGL();

	private:
		RenderState& m_renderState;
		OpenGL* m_gl;

		bool m_scissorEnabled = false;
		bool m_blendEnabled = false;
		MaterialBlendMode m_activeBlendMode = (MaterialBlendMode)-1;

		Set<Material> m_initializedShaders;
		Mesh m_currentMesh;
		Material m_currentMaterial;
	};

	/*
		Base class for renderable items inside a RenderQueue
	*/
	class RenderQueueItem
	{
	public:
		virtual ~RenderQueueItem() = default;
		virtual void Draw(RenderQueueState& state) = 0;
	};

	// Most basic draw command that contains a material, it's parameters and a world transform
	class SimpleDrawCall : public RenderQueueItem
	{
	public:
		SimpleDrawCall();
		// The mesh to draw
		Mesh mesh;
		// Material to use
		Material mat;
		MaterialParameterSet params;
		// The world transform
		Transform worldTransform;
		// Scissor rectangle
		Rect scissorRect;

		virtual void Draw(RenderQueueState& state) override;
	};

	// Command for points/lines with size/width parameter
	class PointDrawCall : public RenderQueueItem
	{
	public:
		// List of points/lines
		Mesh mesh;
		Material mat;
		MaterialParameterSet params;
		// Line or point size (based on the mesh)
		float size;
		// The world transform
		Transform worldTransform;

		virtual void Draw(RenderQueueState& state) override;
	};
}