#include "stdafx.h"
#include "Test.hpp"
#include "Application.hpp"
#include "Profiling.hpp"
#include "Scoring.hpp"
#include "Audio.hpp"
#include "Track.hpp"
#include "Camera.hpp"
#include "Background.hpp"

#include <Shared/Variant.hpp>
#include <Shared/Config.hpp>

class Test_Impl : public Test
{
private:
	Timer t;

public:
	bool Init()
	{
		return true;
	}
	virtual void Render(float deltaTime) override
	{
		RenderState rs = g_application->GetRenderStateBase();
		rs.cameraTransform *= Transform::Rotation(-Vector3(-20.0f, 0.0f, 0.0f));
		rs.cameraTransform *= Transform::Translation(-Vector3(0.0f, 10.0f, 20.0f));
		rs.projectionTransform = ProjectionMatrix::CreatePerspective(40.0f, g_aspectRatio, 0.1f, 100.0f);
		RenderQueue rq(g_gl, rs);

		glClearColor(0.0f, 0.0f, 0.0f, 0);
		glClear(GL_COLOR_BUFFER_BIT);
		rq.Process();
	}
};

Test* Test::Create()
{
	Test_Impl* impl = new Test_Impl();
	if(!impl->Init())
	{
		delete impl;
		return nullptr;
	}
	return impl;
}
