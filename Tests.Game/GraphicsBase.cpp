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
		float dt = t.SecondsAsFloat();
		t.Restart();
		Render(dt);

		// Garbage collect resources
		ResourceManagers::TickAll();
	}

	return true;
}
