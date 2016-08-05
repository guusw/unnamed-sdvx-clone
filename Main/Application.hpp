#pragma once
#include "Sample.hpp"

extern class OpenGL* g_gl;
extern class Window* g_gameWindow;
extern float g_aspectRatio;
extern Vector2i g_resolution;
extern class Application* g_application;
extern class Game* g_game;
extern class Config g_mainConfig;
extern class JobSheduler* g_jobSheduler;

class Application
{
public:
	Application();
	~Application();

	// Runs the application
	int32 Run();

	// Tries to launch a new game window for specified map
	//	doesn't work if a game is already in progress
	//	window gets added using AddTickable if this function succeeds
	bool LaunchMap(const String& mapPath);
	// Shuts down the game window and removes it using RemoveTickable
	void CleanupGame();
	void CleanupMap();
	void Shutdown();

	void AddTickable(class IApplicationTickable* tickable);
	void RemoveTickable(class IApplicationTickable* tickable);

	// Current running map path (full file path)
	String GetCurrentMapPath();

	// Retrieves application command line parameters
	const Vector<String>& GetAppCommandLine() const;

	// Gets a basic template for a render state, with all the application variables initialized
	RenderState GetRenderStateBase() const;

	// Sets FPS limit, <= 0 for unlimited
	void SetFrameLimiter(int32 fpsCap);

	Texture LoadTexture(const String& name);
	Material LoadMaterial(const String& name);
	Sample LoadSample(const String& name);

	float GetAppTime() const { return m_lastUpdateTime; }
	float GetUpdateFPS() const;
	float GetRenderFPS() const;

	Transform GetGUIProjection() const;

private:
	bool m_LoadConfig();
	void m_LoadDefaultConfig();
	void m_SaveConfig();

	bool m_Init();
	void m_MainLoop();
	void m_Cleanup();
	void m_OnKeyPressed(Key key);
	void m_OnKeyReleased(Key key);
	void m_OnWindowResized(const Vector2i& newSize);

	RenderState m_renderStateBase;
	Vector<String> m_commandLine;

	String m_lastMapPath;
	class Beatmap* m_currentMap = nullptr;

	float m_lastUpdateTime;
	float m_lastRenderTime;
	float m_deltaTime;
	bool m_allowMapConversion;
};