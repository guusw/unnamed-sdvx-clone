#include "stdafx.h"
#include "GraphicsBase.hpp"
#include "Graphics/ResourceManagers.hpp"

GraphicsTestBase::~GraphicsTestBase()
{
	if(m_gl)
		delete m_gl;
	if(m_window)
		delete m_window;
}

bool GraphicsTestBase::Run()
{
	m_window = new Graphics::Window();

	m_gl = new OpenGL();
	if(!m_gl->Init(*m_window))
	{
		delete m_gl;
		m_gl = nullptr;
		return false;
	}
	m_window->Show();

	Init();

	Timer t;
	while(m_window->Update())
	{
		// Clear
		Vector2 windowSize = m_window->GetWindowSize();
		m_gl->SetViewport(Recti(Vector2i(), windowSize));
		glClearColor(0.1f, 0.1f, 0.1f, 0.1f);
		glClear(GL_COLOR_BUFFER_BIT);

		float dt = t.SecondsAsFloat();
		if(dt > 1.0f / 60.0f)
			dt = 1.0f / 60.0f; // Limit delta-time for step debugging
		t.Restart();
		Render(dt);

		m_gl->SwapBuffers();

		// Garbage collect resources
		ResourceManagers::TickAll();
	}

	return true;
}
void GUITestBase::Render(float deltaTime)
{
	gui->Update(deltaTime);
	gui->Render(guiRenderer.GetData());
}
