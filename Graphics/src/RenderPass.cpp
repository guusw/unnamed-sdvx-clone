#include "stdafx.h"
#include "RenderPass.hpp"
#include "OpenGL.hpp"

namespace Graphics
{
	void RenderPass::Draw(RenderQueueState& state)
	{
		assert(framebuffer);
		auto& gl = state.GetGL();

		// Store viewport
		Recti oldViewport = gl.GetViewport();

		// Set viewport to match framebuffer
		Recti newViewport = Recti(Vector2i(0), framebuffer->GetSize());
		gl.SetViewport(newViewport);

		framebuffer->Bind();
		{
			glClearColor(clearColor.x, clearColor.y, clearColor.z, clearColor.w);
			if(clearColorEnabled)
				glClear(GL_COLOR_BUFFER_BIT);

			Process(true);
		}
		framebuffer->Unbind();

		// Restore viewport
		gl.SetViewport(oldViewport);

		state.Reset();
	}
}