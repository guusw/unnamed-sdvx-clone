#include "stdafx.h"
#include "RenderQueue.hpp"
#include "OpenGL.hpp"
using Utility::Cast;

RenderQueue::RenderQueue(OpenGL* ogl, const RenderState& rs)
{
	m_ogl = ogl;
	m_renderState = rs;
}
RenderQueue::RenderQueue(RenderQueue&& other)
{
	m_ogl = other.m_ogl;
	other.m_ogl = nullptr;
	m_orderedCommands = move(other.m_orderedCommands);
	m_renderState = other.m_renderState;
}
RenderQueue& RenderQueue::operator=(RenderQueue&& other)
{
	Clear();
	m_ogl = other.m_ogl;
	other.m_ogl = nullptr;
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
	assert(m_ogl);

	// Create a new list of items
 	for(RenderQueueItem* item : m_orderedCommands)
	{
		auto SetupMaterial = [&](Material& mat, MaterialParameterSet& params)
		{
			mat->Bind(m_renderState, params);

			// Setup Render state for transparent object
			if(mat->opaque)
			{
				glDisable(GL_BLEND);
			}
			else
			{
				glEnable(GL_BLEND);
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
		};

		if(Cast<SimpleDrawCall>(item))
		{
			SimpleDrawCall* sdc = (SimpleDrawCall*)item;
			m_renderState.worldTransform = sdc->worldTransform;
			SetupMaterial(sdc->mat, sdc->params);
			sdc->mesh->Draw();
		}
		else if(Cast<PointDrawCall>(item))
		{
			PointDrawCall* pdc = (PointDrawCall*)item;
			m_renderState.worldTransform = Transform();
			SetupMaterial(pdc->mat, pdc->params);
			PrimitiveType pt = pdc->mesh->GetPrimitiveType();
			if(pt >= PrimitiveType::LineList || pt <= PrimitiveType::LineStrip)
			{
				glLineWidth(pdc->size);
			}
			else
			{
				glPointSize(pdc->size);
			}
			pdc->mesh->Draw();
		}
	}

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

void RenderQueue::Draw(Transform worldTransform, Mesh m, Material mat, const MaterialParameterSet& params)
{
	SimpleDrawCall* sdc = new SimpleDrawCall();
	sdc->mat = mat;
	sdc->mesh = m;
	sdc->params = params;
	sdc->worldTransform = worldTransform;
	m_orderedCommands.push_back(sdc);
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
	m_orderedCommands.push_back(sdc);
}
void RenderQueue::DrawPoints(Mesh m, Material mat, const MaterialParameterSet& params, float pointSize)
{
	PointDrawCall* pdc = new PointDrawCall();
	pdc->mat = mat;
	pdc->mesh = m;
	pdc->params = params;
	pdc->size = pointSize;
	m_orderedCommands.push_back(pdc);
}
