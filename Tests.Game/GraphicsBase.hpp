#pragma once

#include <Graphics/Graphics.hpp>
using namespace Graphics;

class GraphicsTestBase : public Unique
{
public:
	~GraphicsTestBase();
	virtual void Init() {};
	virtual void Render(float deltaTime) = 0;
	bool Run();

protected:
	Graphics::Window* m_window = nullptr;
	Graphics::OpenGL* m_gl = nullptr;
};