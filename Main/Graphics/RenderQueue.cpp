#include "stdafx.h"
#include "RenderQueue.hpp"
#include "OpenGL.hpp"
using Utility::Cast;

RenderQueue::RenderQueue(OpenGL* ogl, const RenderState& rs)
{
	m_ogl = ogl;
	m_renderState = rs;
}
void RenderQueue::Process()
{
	// Create a new list of items
 	for(RenderQueueItem* item : m_orderedCommands)
	{
		if(Cast<SimpleDrawCall>(item))
		{
			SimpleDrawCall* sdc = (SimpleDrawCall*)item;
			m_renderState.worldTransform = sdc->worldTransform;
			sdc->mat->Bind(m_renderState, sdc->params);

			// Setup Render state for transparent object
			if(sdc->mat->opaque)
			{
				glDisable(GL_BLEND);
			}
			else
			{
				glEnable(GL_BLEND);
				switch(sdc->mat->blendMode)
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

			sdc->mesh->Draw();
		}
	}

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
