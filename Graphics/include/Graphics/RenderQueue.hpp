#pragma once
#include <Graphics/RenderItem.hpp>

namespace Graphics
{
	/*
		This class is a queue that collects draw commands
		each of these is stored together with their wanted render state.

		When Process is called, the commands are sorted and grouped, then sent to the graphics pipeline.
	*/
	class RenderQueue : public Unique
	{
	public:
		RenderQueue() = default;
		RenderQueue(OpenGL* ogl, const RenderState& rs);
		RenderQueue(RenderQueue&& other);
		RenderQueue& operator=(RenderQueue&& other);
		~RenderQueue();
		// Processes all render commands
		void Process(bool clearQueue = true);
		// Clears all the render commands in the queue
		void Clear();

		// Adds a new item to the queue
		// Passing in a pointer which will be removed by the queue after being processed or destroyed
		void Add(RenderQueueItem* item);

		void Draw(Transform worldTransform, Mesh m, Material mat, const MaterialParameterSet& params = MaterialParameterSet());
		void Draw(Transform worldTransform, Ref<class TextRes> text, Material mat, const MaterialParameterSet& params = MaterialParameterSet());
		void DrawScissored(Rect scissor, Transform worldTransform, Mesh m, Material mat, const MaterialParameterSet& params = MaterialParameterSet());
		void DrawScissored(Rect scissor, Transform worldTransform, Ref<class TextRes> text, Material mat, const MaterialParameterSet& params = MaterialParameterSet());

		// Draw for lines/points with point size parameter
		void DrawPoints(Mesh m, Material mat, const MaterialParameterSet& params, float pointSize);

	protected:
		RenderState m_renderState;
		Vector<RenderQueueItem*> m_orderedCommands;
		class OpenGL* m_gl = nullptr;
	};
}