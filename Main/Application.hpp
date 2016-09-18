#pragma once
#include <Audio/Sample.hpp>

extern class OpenGL* g_gl;
extern class Graphics::Window* g_gameWindow;
extern float g_aspectRatio;
extern Vector2i g_resolution;
extern class Application* g_application;
extern class JobSheduler* g_jobSheduler;
extern class Input g_input;

// GUI
extern class GUIRenderer* g_guiRenderer;
extern Ref<class Canvas> g_rootCanvas;

class Application
{
public:
	Application();
	~Application();

	// Runs the application
	int32 Run();
	
	void SetCommandLine(int32 argc, char** argv);
	void SetCommandLine(const char* cmdLine);

	class Game* LaunchMap(const String& mapPath);
	void Shutdown();

	void AddTickable(class IApplicationTickable* tickable, class IApplicationTickable* insertBefore = nullptr);
	void RemoveTickable(class IApplicationTickable* tickable);

	// Current running map path (full file path)
	String GetCurrentMapPath();

	// Retrieves application command line parameters
	const Vector<String>& GetAppCommandLine() const;

	// Gets a basic template for a render state, with all the application variables initialized
	RenderState GetRenderStateBase() const;

#ifdef LoadImage
#undef LoadImage
#endif
	Image LoadImage(const String& name);
	Texture LoadTexture(const String& name);
	Material LoadMaterial(const String& name);
	Sample LoadSample(const String& name);

	float GetAppTime() const { return m_lastRenderTime; }
	float GetRenderFPS() const;

	Transform GetGUIProjection() const;

private:
	bool m_LoadConfig();
	void m_SaveConfig();

	bool m_Init();
	void m_MainLoop();
	void m_Tick();

	void m_Cleanup();
	void m_OnKeyPressed(Key key);
	void m_OnKeyReleased(Key key);
	void m_OnWindowResized(const Vector2i& newSize);

	RenderState m_renderStateBase;
	Vector<String> m_commandLine;

	String m_lastMapPath;
	class Beatmap* m_currentMap = nullptr;

	float m_lastRenderTime;
	float m_deltaTime;
	bool m_allowMapConversion;
};