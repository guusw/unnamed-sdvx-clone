#pragma once
#include <Graphics/Window.hpp>

void APIENTRY GLDebugProc(GLenum source, GLenum type, GLuint id, GLenum severity, GLsizei length, const GLchar* message, const void* userParam);

/*
	OpenGL context wrapper with common functionality
*/
class OpenGL
{
	class ShaderRes* m_activeShaders[3] = { 0 };
	uint32 m_mainProgramPipeline;
	HDC m_deviceContext = 0;
	HGLRC m_renderContext = 0;
	Window* m_window;
	class FramebufferRes* m_boundFramebuffer;

	friend class ShaderRes;
	friend class TextureRes;
	friend class MeshRes;
	friend class Shader_Impl;
	friend class Framebuffer_Impl;

public:
	OpenGL();
	~OpenGL();
	void InitResourceManagers();
	bool Init(Window& window);
	void UnbindFramebuffer();

	Recti GetViewport() const;
	void SetViewport(Vector2i size);
	void SetViewport(Recti vp);

	virtual void SwapBuffers();
};