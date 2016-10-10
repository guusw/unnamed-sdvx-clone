#include "stdafx.h"
#include "RenderQueue.hpp"

namespace Graphics
{
	RenderQueueState::RenderQueueState(RenderState& renderState, OpenGL* gl) 
		: m_gl(gl), m_renderState(renderState)
	{
	}
	RenderQueueState::~RenderQueueState()
	{
		// Disable all states might have been on
		glDisable(GL_BLEND);
		glDisable(GL_SCISSOR_TEST);
		glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	}
	void RenderQueueState::SetMaterial(Material& mat, MaterialParameterSet& params)
	{
		assert(mat);

		// Only bind params if material is already bound to context
		if(m_currentMaterial == mat)
			mat->BindParameters(params, m_renderState.worldTransform);
		else
		{
			if(m_initializedShaders.Contains(mat))
			{
				// Only bind params and rebind
				mat->BindParameters(params, m_renderState.worldTransform);
				mat->BindToContext();
				m_currentMaterial = mat;
			}
			else
			{
				mat->Bind(m_renderState, params);
				m_initializedShaders.Add(mat);
				m_currentMaterial = mat;
			}
		}

		// Setup Render state for transparent object
		if(mat->opaque)
		{
			if(m_blendEnabled)
			{
				glDisable(GL_BLEND);
				m_blendEnabled = false;
			}
		}
		else
		{
			if(!m_blendEnabled)
			{
				glEnable(GL_BLEND);
				m_blendEnabled = true;
			}
			if(m_activeBlendMode != mat->blendMode)
			{
				switch(mat->blendMode)
				{
				case MaterialBlendMode::Normal:
					glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
					break;
				case MaterialBlendMode::Additive:
					glBlendFunc(GL_SRC_ALPHA, GL_ONE);
					break;
				case MaterialBlendMode::Multiply:
					glBlendFunc(GL_SRC_ALPHA, GL_SRC_COLOR);
					break;
				}
			}
		}
	}
	void RenderQueueState::DrawOrRedrawMesh(Mesh& mesh)
	{
		if(m_currentMesh == mesh)
			mesh->Redraw();
		else
		{
			mesh->Draw();
			m_currentMesh = mesh;
		}
	}
	void RenderQueueState::SetWorldTransform(Transform& transform)
	{
		m_renderState.worldTransform = transform;
	}

	void RenderQueueState::SetScissorEnabled(bool enabled)
	{
		if(enabled != m_scissorEnabled)
		{
			// Apply scissor
			if(!enabled)
			{
				glEnable(GL_SCISSOR_TEST);
				m_scissorEnabled = true;
			}
			else // Disable scissor
			{
				glDisable(GL_SCISSOR_TEST);
				m_scissorEnabled = false;
			}
		}
	}
	void RenderQueueState::SetScissorRect(const Rect& rect)
	{
		float scissorY = m_renderState.viewportSize.y - rect.Bottom();
		glScissor((int32)rect.Left(), (int32)scissorY,
			(int32)rect.size.x, (int32)rect.size.y);
	}

	void RenderQueueState::Reset()
	{
		m_currentMesh.Release();
		m_initializedShaders.clear();
		m_currentMaterial.Release();
		m_blendEnabled = false;
		m_activeBlendMode = MaterialBlendMode::Normal;

		// Disable all states might have been on
		glDisable(GL_BLEND);
		glDisable(GL_SCISSOR_TEST);
		glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	}

	Graphics::RenderState& RenderQueueState::GetRenderState()
	{
		return m_renderState;
	}

	Graphics::OpenGL& RenderQueueState::GetGL()
	{
		return *m_gl;
	}

	void SimpleDrawCall::Draw(RenderQueueState& state)
	{
		state.SetWorldTransform(worldTransform);
		state.SetMaterial(mat, params);

		// Check if scissor is enabled
		bool useScissor = (scissorRect.size.x >= 0);
		state.SetScissorEnabled(useScissor);
		if(useScissor)
		{
			state.SetScissorRect(scissorRect);
		}

		state.DrawOrRedrawMesh(mesh);
	}

	void PointDrawCall::Draw(RenderQueueState& state)
	{
		state.SetScissorEnabled(false);
		state.SetWorldTransform(worldTransform);
		state.SetMaterial(mat, params);

		PrimitiveType pt = mesh->GetPrimitiveType();
		if(pt >= PrimitiveType::LineList && pt <= PrimitiveType::LineStrip)
		{
			glLineWidth(size);
		}
		else
		{
			glPointSize(size);
		}

		state.DrawOrRedrawMesh(mesh);
	}


}