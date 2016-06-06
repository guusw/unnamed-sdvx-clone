#include "stdafx.h"
#include "OpenGL.hpp"
#include <Graphics/ResourceManager.hpp>
#pragma comment(lib, "opengl32.lib")

#include "Mesh.hpp"
#include "Texture.hpp"
#include "Shader.hpp"
#include "Font.hpp"
#include "Material.hpp"
#include "Framebuffer.hpp"
#include "ParticleSystem.hpp"

OpenGL::OpenGL()
{

}
OpenGL::~OpenGL()
{
	if(m_renderContext)
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

		//ResourceManagers::DestroyAll();
		wglMakeCurrent(m_deviceContext, 0);
		wglDeleteContext(m_renderContext);
		m_renderContext = 0;
	}
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
	m_window = &window;
	m_deviceContext = GetDC((HWND)m_window->Handle());

	PIXELFORMATDESCRIPTOR pfd;
	memset(&pfd, 0, sizeof(pfd));
	pfd.nSize = sizeof(pfd);
	pfd.nVersion = 1;
	pfd.dwFlags = PFD_DRAW_TO_WINDOW | PFD_SUPPORT_OPENGL | PFD_DOUBLEBUFFER;
	pfd.iPixelType = PFD_TYPE_RGBA;
	pfd.iLayerType = PFD_MAIN_PLANE;
	pfd.cDepthBits = 0;
	int nPixelFormat = ChoosePixelFormat(m_deviceContext, &pfd);
	if(nPixelFormat == 0)
		return false;

	if(!SetPixelFormat(m_deviceContext, nPixelFormat, &pfd))
		return false;

	HGLRC intCtx = wglCreateContext(m_deviceContext);
	if(!intCtx)
	{
		Log("Failed to create intermediate OpenGL context", Logger::Error);
		return false;
	}
	if(!wglMakeCurrent(m_deviceContext, intCtx))
	{
		Log("Failed to set current context", Logger::Error);
		return false;
	}

	wglCreateContextAttribsARB = (PFNWGLCREATECONTEXTATTRIBSARBPROC)wglGetProcAddress("wglCreateContextAttribsARB");
	if(!wglCreateContextAttribsARB)
	{
		Log("wglCreateContextAttribsARB not supported", Logger::Error);
		return false;
	}

	int attribs[] =
	{
		//WGL_CONTEXT_MAJOR_VERSION_ARB, 4,
#ifdef _DEBUG
			WGL_CONTEXT_FLAGS_ARB, WGL_CONTEXT_DEBUG_BIT_ARB,
#endif
			WGL_CONTEXT_PROFILE_MASK_ARB, WGL_CONTEXT_CORE_PROFILE_BIT_ARB,
			0, 0
	};

	m_renderContext = wglCreateContextAttribsARB(m_deviceContext, 0, attribs);
	if(!m_renderContext)
	{
		Log("Failed to create OpenGL 4 core context", Logger::Error);
	}

	// Set current context and reaquire function pointeres using glew
	wglMakeCurrent(m_deviceContext, m_renderContext);
	glewInit();

	// Delete intermediate context
	wglDeleteContext(intCtx);

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

	wglSwapIntervalEXT(0);

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
	::SwapBuffers(m_deviceContext);
}

void APIENTRY GLDebugProc(GLenum source, GLenum type, GLuint id, GLenum severity, GLsizei length, const GLchar* message, const void* userParam)
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