#include "stdafx.h"
#include "OpenGL.hpp"
#include <Graphics/ResourceManagers.hpp>
#ifdef _MSC_VER
#pragma comment(lib, "opengl32.lib")
#endif

#include "Mesh.hpp"
#include "Texture.hpp"
#include "Shader.hpp"
#include "Font.hpp"
#include "Material.hpp"
#include "Framebuffer.hpp"
#include "ParticleSystem.hpp"
#include "Window.hpp"

namespace Graphics
{
	class OpenGL_Impl
	{
	public:
		SDL_GLContext context;
	};

	OpenGL::OpenGL()
	{
		m_impl = new OpenGL_Impl();
	}
	OpenGL::~OpenGL()
	{
		if(m_impl->context)
		{
			// Cleanup resource managers
			ResourceManagers::DestroyResourceManager<ResourceType::Mesh>();
			ResourceManagers::DestroyResourceManager<ResourceType::Texture>();
			ResourceManagers::DestroyResourceManager<ResourceType::Shader>();
			ResourceManagers::DestroyResourceManager<ResourceType::Font>();
			ResourceManagers::DestroyResourceManager<ResourceType::Material>();
			ResourceManagers::DestroyResourceManager<ResourceType::Framebuffer>();
			ResourceManagers::DestroyResourceManager<ResourceType::ParticleSystem>();

			if(glBindProgramPipeline)
			{
				glDeleteProgramPipelines(1, &m_mainProgramPipeline);
			}

			SDL_GL_DeleteContext(m_impl->context);
			m_impl->context = nullptr;
		}
		delete m_impl;
	}
	void OpenGL::InitResourceManagers()
	{
		ResourceManagers::CreateResourceManager<ResourceType::Mesh>();
		ResourceManagers::CreateResourceManager<ResourceType::Texture>();
		ResourceManagers::CreateResourceManager<ResourceType::Shader>();
		ResourceManagers::CreateResourceManager<ResourceType::Font>();
		ResourceManagers::CreateResourceManager<ResourceType::Material>();
		ResourceManagers::CreateResourceManager<ResourceType::Framebuffer>();
		ResourceManagers::CreateResourceManager<ResourceType::ParticleSystem>();
	}
	bool OpenGL::Init(Window& window)
	{
		if(m_impl->context)
			return true; // Already initialized

		m_window = &window;
		SDL_Window* sdlWnd = (SDL_Window*)m_window->Handle();

		// Create a context
		m_impl->context = SDL_GL_CreateContext(sdlWnd);
		glewInit();

		//#define LIST_OGL_EXTENSIONS
#ifdef LIST_OGL_EXTENSIONS
		PRINT("Listing OpenGL Extensions:");
		GLint n, i;
		glGetIntegerv(GL_NUM_EXTENSIONS, &n);
		for(i = 0; i < n; i++) {
			printf("%s\n", glGetStringi(GL_EXTENSIONS, i));
		}
#endif

#ifdef _DEBUG
		glDebugMessageCallback(GLDebugProc, 0);
		glDebugMessageControl(GL_DONT_CARE, GL_DONT_CARE, GL_DONT_CARE, 0, 0, GL_TRUE);
		glDebugMessageControl(GL_DONT_CARE, GL_DEBUG_TYPE_OTHER, GL_DONT_CARE, 0, 0, GL_FALSE);
#endif

		// Disable VSync
		//	Framerate is to be limited by the application manually
		SDL_GL_SetSwapInterval(0);

		InitResourceManagers();

		// Create pipeline for the program
		glGenProgramPipelines(1, &m_mainProgramPipeline);
		glBindProgramPipeline(m_mainProgramPipeline);

		glDisable(GL_DEPTH_TEST);
		glEnable(GL_CULL_FACE);
		glEnable(GL_TEXTURE_2D);
		glDisable(GL_BLEND);

		return true;
	}

	void OpenGL::UnbindFramebuffer()
	{
		if(m_boundFramebuffer)
		{
			m_boundFramebuffer->Unbind();
		}
	}

	Recti OpenGL::GetViewport() const
	{
		Recti vp;
		glGetIntegerv(GL_VIEWPORT, &vp.pos.x);
		return vp;
	}
	void OpenGL::SetViewport(Recti vp)
	{
		glViewport(vp.pos.x, vp.pos.y, vp.size.x, vp.size.y);
	}
	void OpenGL::SetViewport(Vector2i size)
	{
		glViewport(0, 0, size.x, size.y);
	}

	void OpenGL::SwapBuffers()
	{
		glFlush();
		SDL_Window* sdlWnd = (SDL_Window*)m_window->Handle();
		SDL_GL_SwapWindow(sdlWnd);
	}

	#ifdef _WIN32
	void APIENTRY GLDebugProc(GLenum source, GLenum type, GLuint id, GLenum severity, GLsizei length, const GLchar* message, const void* userParam)
	#else
	void GLDebugProc(GLenum source, GLenum type, GLuint id, GLenum severity, GLsizei length, const GLchar* message, const void* userParam)
	#endif
	{
#define DEBUG_SEVERITY_HIGH                              0x9146
#define DEBUG_SEVERITY_MEDIUM                            0x9147
#define DEBUG_SEVERITY_LOW                               0x9148
#define DEBUG_SEVERITY_NOTIFICATION                      0x826B

		Logger::Severity mySeverity;
		switch(severity)
		{
		case DEBUG_SEVERITY_MEDIUM:
		case DEBUG_SEVERITY_HIGH:
			mySeverity = Logger::Warning;
			break;
		default:
			mySeverity = Logger::Info;
			break;
		}
		String msgString = String(message, message + length);
		Logf("GLDebug: %s", Logger::Info, msgString);
	}
}