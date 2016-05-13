#pragma once
#include "Texture.hpp"
#include "Material.hpp"
#include "Sample.hpp"
#include "RenderQueue.hpp"
#include "RenderState.hpp"

extern class OpenGL* g_gl;
extern class Window* g_gameWindow;
extern float g_aspectRatio;
extern Vector2i g_resolution;
extern class Application* g_application;
extern class Game* g_game;

class Application
{
public:
	Application();
	~Application();
	// Runs the application
	int32 Run();

	bool LaunchMap(const String& mapPath);
	void Shutdown();

	// Retrieves application command line parameters
	const Vector<String>& GetAppCommandLine() const;

	// Gets a basic template for a render state, with all the application variables initialized
	RenderState GetRenderStateBase() const;

	Texture LoadTexture(const String& name);
	Material LoadMaterial(const String& name);
	Sample LoadSample(const String& name);

	float GetAppTime() const { return m_lastUpdateTime; }

	Transform GetGUIProjection() const;

private:
	void m_Cleanup();
	void m_OnKeyPressed(uint8 key);
	void m_OnKeyReleased(uint8 key);
	void m_OnWindowResized(const Vector2i& newSize);

	RenderState m_renderStateBase;
	Vector<String> m_commandLine;
	class Beatmap* m_currentMap = nullptr;
	float m_lastUpdateTime;
	float m_lastRenderTime;
	float m_deltaTime;
	bool m_allowMapConversion;
};