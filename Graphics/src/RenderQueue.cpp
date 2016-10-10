#include "stdafx.h"
#include "RenderQueue.hpp"
#include "OpenGL.hpp"
using Utility::Cast;

namespace Graphics
{
	RenderState RenderState::Create2DRenderState(Recti viewport)
	{
		RenderState guiRs;
		guiRs.viewportSize = viewport.size;
		guiRs.projectionTransform = ProjectionMatrix::CreateOrthographic((float)viewport.Left(), (float)viewport.Right(), (float)viewport.Bottom(), (float)viewport.Top(), -1.0f, 100.0f);
		guiRs.aspectRatio = (float)viewport.size.y / (float)viewport.size.x;
		return guiRs;
	}

	RenderQueue::RenderQueue(OpenGL* ogl, const RenderState& rs)
	{
		m_gl = ogl;
		m_renderState = rs;
	}
	RenderQueue::RenderQueue(RenderQueue&& other)
	{
		m_gl = other.m_gl;
		other.m_gl = nullptr;
		m_orderedCommands = move(other.m_orderedCommands);
		m_renderState = other.m_renderState;
	}
	RenderQueue& RenderQueue::operator=(RenderQueue&& other)
	{
		Clear();
		m_gl = other.m_gl;
		other.m_gl = nullptr;
		m_orderedCommands = move(other.m_orderedCommands);
		m_renderState = other.m_renderState;
		return *this;
	}
	RenderQueue::~RenderQueue()
	{
		Clear();
	}
	void RenderQueue::Process(bool clearQueue)
	{
		assert(m_gl);

		RenderQueueState* state = new RenderQueueState(m_renderState, m_gl);
		{
			// Render all the items
			for(RenderQueueItem* item : m_orderedCommands)
			{
				item->Draw(*state);
			}
		}
		delete state;

		if(clearQueue)
		{
			Clear();
		}
	}

	void RenderQueue::Clear()
	{
		// Cleanup the list of items
		for(RenderQueueItem* item : m_orderedCommands)
		{
			delete item;
		}
		m_orderedCommands.clear();
	}

	void RenderQueue::Add(RenderQueueItem* item)
	{
		m_orderedCommands.Add(item);
	}

	void RenderQueue::Draw(Transform worldTransform, Mesh m, Material mat, const MaterialParameterSet& params)
	{
		SimpleDrawCall* sdc = new SimpleDrawCall();
		sdc->mat = mat;
		sdc->mesh = m;
		sdc->params = params;
		sdc->worldTransform = worldTransform;
		Add(sdc);
	}
	void RenderQueue::Draw(Transform worldTransform, Ref<class TextRes> text, Material mat, const MaterialParameterSet& params)
	{
		SimpleDrawCall* sdc = new SimpleDrawCall();
		sdc->mat = mat;
		sdc->mesh = text->GetMesh();
		sdc->params = params;
		// Set Font texture map
		sdc->params.SetParameter("mainTex", text->GetTexture());
		sdc->worldTransform = worldTransform;
		Add(sdc);
	}

	void RenderQueue::DrawScissored(Rect scissor, Transform worldTransform, Mesh m, Material mat, const MaterialParameterSet& params /*= MaterialParameterSet()*/)
	{
		SimpleDrawCall* sdc = new SimpleDrawCall();
		sdc->mat = mat;
		sdc->mesh = m;
		sdc->params = params;
		sdc->worldTransform = worldTransform;
		sdc->scissorRect = scissor;
		Add(sdc);
	}
	void RenderQueue::DrawScissored(Rect scissor, Transform worldTransform, Ref<class TextRes> text, Material mat, const MaterialParameterSet& params /*= MaterialParameterSet()*/)
	{
		SimpleDrawCall* sdc = new SimpleDrawCall();
		sdc->mat = mat;
		sdc->mesh = text->GetMesh();
		sdc->params = params;
		// Set Font texture map
		sdc->params.SetParameter("mainTex", text->GetTexture());
		sdc->worldTransform = worldTransform;
		sdc->scissorRect = scissor;
		Add(sdc);
	}

	void RenderQueue::DrawPoints(Mesh m, Material mat, const MaterialParameterSet& params, float pointSize)
	{
		PointDrawCall* pdc = new PointDrawCall();
		pdc->mat = mat;
		pdc->mesh = m;
		pdc->params = params;
		pdc->size = pointSize;
		Add(pdc);
	}

	// Initializes the simple draw call structure
	SimpleDrawCall::SimpleDrawCall()
		: scissorRect(Vector2(), Vector2(-1))
	{
	}

}
