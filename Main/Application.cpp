#include "stdafx.h"
#include "Application.hpp"
#include "OpenGL.hpp"
#include "Image.hpp"
#include "Window.hpp"
#include "Beatmap.hpp"
#include "Game.hpp"
#include "Audio.hpp"
#include "ResourceManager.hpp"
#include "Profiling.hpp"

OpenGL* g_gl = nullptr;
Window* g_gameWindow = nullptr;
Application* g_application = nullptr;
Game* g_game = nullptr;
// Use rotated 16:9 as aspect ratio
float g_aspectRatio = 1.0f / (16.0f / 9.0f);
static float g_screenHeight = 1000.0f;
Vector2i g_resolution;

Application::Application()
{
	// Enforce single instance
	assert(!g_application);
	g_application = this;
}
Application::~Application()
{
	m_Cleanup();
	assert(g_application == this);
	g_application = nullptr;
}
int32 Application::Run()
{
	{
		ProfilerScope $("Application Setup");

		// Split up command line parameters
		String cmdLine = Utility::ConvertToANSI(GetCommandLine());
		m_commandLine = Path::SplitCommandLine(cmdLine);
		assert(m_commandLine.size() >= 1);

		m_allowMapConversion = false;
		bool debugMute = false;
		for(auto& cl : m_commandLine)
		{
			if(cl == "-convertmaps")
			{
				m_allowMapConversion = true;
			}
			else if(cl == "-mute")
			{
				debugMute = true;
			}
		}

		// Create the game window
		g_resolution = Vector2i{ (int32)(g_screenHeight * g_aspectRatio), (int32)g_screenHeight };
		g_gameWindow = new Window(g_resolution);
		g_gameWindow->Show();
		g_gameWindow->OnKeyPressed.Add(this, &Application::m_OnKeyPressed);
		g_gameWindow->OnKeyReleased.Add(this, &Application::m_OnKeyReleased);
		g_gameWindow->OnResized.Add(this, &Application::m_OnWindowResized);

		// Fixed window style
		g_gameWindow->UnsetStyles(WS_SIZEBOX | WS_MAXIMIZE | WS_MAXIMIZEBOX);

		// Set render state variables
		m_renderStateBase.aspectRatio = g_aspectRatio;
		m_renderStateBase.viewportSize = g_resolution;
		m_renderStateBase.time = 0.0f;

		{
			ProfilerScope $1("Audio Init");

			// Init audio
			new Audio();
			if(!g_audio->Init(*g_gameWindow))
			{
				Log("Audio initialization failed", Logger::Error);
				delete g_audio;
				return 1;
			}

			// Debug Mute?
			// Test tracks may get annoying when continuosly debugging ;)
			if(debugMute)
			{
				g_audio->SetGlobalVolume(0.0f);
			}
		}

		{
			ProfilerScope $1("GL Init");

			// Create graphics context
			g_gl = new OpenGL();
			if(!g_gl->Init(*g_gameWindow))
			{
				Log("Failed to create OpenGL context", Logger::Error);
				return 1;
			}
		}

		// Setup the game by processing the command line
		if(m_commandLine.size() < 2)
		{
			Log("No map path specified", Logger::Error);
			return 1;
		}
	}

	// Play the map
	if(!LaunchMap(m_commandLine[1]))
		return 1;

	Timer appTimer;
	m_lastRenderTime = 0.0f;
	m_lastUpdateTime = 0.0f;
	while(true)
	{
		static const float maxDeltaTime = (1.0f / 30.0f);

		// Gameplay loop
		/// TODO: Add timing management
		for(uint32 i = 0; i < 32; i++)
		{
			// Input update
			if(!g_gameWindow->Update())
				return 0;
			// Terminate after game
			if(!g_game || !g_game->IsPlaying())
				return 0;
			float currentTime = appTimer.SecondsAsFloat();
			float deltaTime = Math::Min(currentTime - m_lastUpdateTime, maxDeltaTime);
			m_lastUpdateTime = currentTime;

			g_game->Tick(deltaTime);
		}

		// Set time in render state
		m_renderStateBase.time = m_lastUpdateTime;

		// Render loop
		for(uint32 i = 0; i < 1; i++)
		{
			// Terminate after game
			if(!g_game || !g_game->IsPlaying())
				return 0;

			float currentTime = appTimer.SecondsAsFloat();
			float deltaTime = Math::Min(currentTime - m_lastRenderTime, maxDeltaTime);
			m_lastRenderTime = currentTime;

			// Not minimized and such
			if(g_resolution.x > 0 && g_resolution.y > 0)
			{
				g_game->Render(deltaTime);
				// Swap Front/Back buffer
				g_gl->SwapBuffers();
			}

			// Garbage collect resources
			ResourceManagers::TickAll();
		}
	}

	return 0;
}
void Application::m_Cleanup()
{
	ProfilerScope $("Application Cleanup");
	if(g_game)
	{
		delete g_game;
		g_game = nullptr;
	}

	if(g_audio)
	{
		delete g_audio;
	}

	if(g_gl)
	{
		delete g_gl;
		g_gl = nullptr;
	}

	if(g_gameWindow)
	{
		delete g_gameWindow;
		g_gameWindow = nullptr;
	}
}

// Try load map helper
Beatmap* TryLoadMap(const String& path)
{
	// Load map file
	Beatmap* newMap = new Beatmap();
	File mapFile;
	if(!mapFile.OpenRead(path))
	{
		delete newMap;
		return nullptr;
	}
	FileReader reader(mapFile);
	if(!newMap->Load(reader))
	{
		delete newMap;
		return nullptr;
	}
	return newMap;
}
bool Application::LaunchMap(const String& mapPath)
{
	String actualMapPath = mapPath;

	if(g_game)
	{
		Log("Game already in progress", Logger::Warning);
		return false;
	}

	if(!Path::FileExists(actualMapPath))
	{
		Logf("Couldn't find map at %s", Logger::Error, actualMapPath);
		return false;
	}

	// Check if converted map exists
	String currentExtension = Path::GetExtension(actualMapPath);
	String convertedPath = Path::ReplaceExtension(actualMapPath, ".fxm");
	bool loadedConvertedMap = false;
	if(m_allowMapConversion && currentExtension == "ksh" && Path::FileExists(convertedPath))
	{
		// Try loading converted map
		actualMapPath = convertedPath;
		if(m_currentMap = TryLoadMap(convertedPath))
		{
			loadedConvertedMap = true;
		}
	}
	// Load original map
	if(!loadedConvertedMap)
	{
		m_currentMap = TryLoadMap(actualMapPath);
	}
	
	// Check failure of above loading attempts
	if(!m_currentMap)
		return false;
	// Loaded successfully

	// Save converted map
	if(m_allowMapConversion && !loadedConvertedMap)
	{
		File mapFile;
		mapFile.OpenWrite(convertedPath);
		FileWriter writer(mapFile);
		m_currentMap->Save(writer);
	}

	// Acquire map base path
	String pathCanonical = Path::Canonical(actualMapPath);
	String mapBasePath = Path::RemoveLast(pathCanonical);

	g_game = Game::Create(m_currentMap, mapBasePath);
	if(!g_game)
		return false;

	return true;
}
bool Application::IsPlaying() const
{
	return g_game != nullptr;
}

const Vector<String>& Application::GetAppCommandLine() const
{
	return m_commandLine;
}

RenderState Application::GetRenderStateBase() const
{
	return m_renderStateBase;
}

Texture Application::LoadTexture(const String& name)
{
	String path = String("textures/") + name;
	Image img = ImageRes::Create(path);
	Texture ret = TextureRes::Create(g_gl, img);
	assert(ret);
	return ret;
}
Material Application::LoadMaterial(const String& name)
{
	String pathV = String("shaders/") + name + ".vs";
	String pathF = String("shaders/") + name + ".fs";
	Material ret = MaterialRes::Create(g_gl, pathV, pathF);
	assert(ret);
	return ret;
}

Transform Application::GetGUIProjection() const
{
	return ProjectionMatrix::CreateOrthographic(0.0f, (float)g_resolution.x, (float)g_resolution.y, 0.0f, 0.0f, 100.0f);
}

void Application::m_OnKeyPressed(uint8 key)
{
	if(g_game)
	{
		g_game->OnKeyPressed(key);
	}
	if(key == VK_ESCAPE)
	{
		// Leave game
		delete g_game;
		g_game = nullptr;
	}
}
void Application::m_OnKeyReleased(uint8 key)
{
	if(g_game)
	{
		g_game->OnKeyReleased(key);
	}
}
void Application::m_OnWindowResized(const Vector2i& newSize)
{
	g_resolution = newSize;
	g_aspectRatio = (float)g_resolution.x / (float)g_resolution.y;

	m_renderStateBase.aspectRatio = g_aspectRatio;
	m_renderStateBase.viewportSize = g_resolution;
}