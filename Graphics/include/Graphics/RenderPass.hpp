#pragma once
#include "Graphics/RenderItem.hpp"
#include "Graphics/Framebuffer.hpp"
#include "Graphics/RenderQueue.hpp"

namespace Graphics
{
	/*
		A render to texture item which can also be used as a separate render queue
	*/
	class RenderPass : public RenderQueueItem, public RenderQueue
	{
	public:
		using RenderQueue::RenderQueue;

		// Create a default framebuffer with just a color texture
		void CreateFramebuffer();

		virtual void Draw(RenderQueueState& state) override;

		Framebuffer framebuffer;

		bool clearColorEnabled = true;
		Color clearColor = Color(0.0f, 0.0f, 0.0f, 0.0f);
	};
}