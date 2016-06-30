#include "stdafx.h"
#include "Application.hpp"
#include "Beatmap.hpp"
#include "Game.hpp"
#include "Test.hpp"
#include "Audio.hpp"
#include <Graphics/Window.hpp>
#include <Graphics/ResourceManagers.hpp>
#include "Profiling.hpp"

OpenGL* g_gl = nullptr;
Window* g_gameWindow = nullptr;
Application* g_application = nullptr;

Game* g_game = nullptr;

Vector<IApplicationTickable*> g_tickables;

// Use rotated 16:9 as aspect ratio
float g_aspectRatio = 1.0f /(16.0f / 9.0f);
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
		String cmdLine = Utility::ConvertToUTF8(GetCommandLine());
		m_commandLine = Path::SplitCommandLine(cmdLine);
		assert(m_commandLine.size() >= 1);

		m_allowMapConversion = false;
		bool debugMute = false;
		bool startFullscreen = false;
		uint32 fullscreenMonitor = -1;
		for(auto& cl : m_commandLine)
		{
			String k, v;
			if(cl.Split("=", &k, &v))
			{
				if(k == "-monitor")
				{
					fullscreenMonitor = atol(*v);
				}
			}
			else
			{
				if(cl == "-convertmaps")
				{
					m_allowMapConversion = true;
				}
				else if(cl == "-mute")
				{
					debugMute = true;
				}
				else if(cl == "-fullscreen")
				{
					startFullscreen = true;
				}
			}
		}

		// Create the game window
		g_resolution = Vector2i{ (int32)(g_screenHeight * g_aspectRatio), (int32)g_screenHeight };
		g_gameWindow = new Window(g_resolution);
		g_gameWindow->Show();
		m_OnWindowResized(g_resolution);
		g_gameWindow->OnKeyPressed.Add(this, &Application::m_OnKeyPressed);
		g_gameWindow->OnKeyReleased.Add(this, &Application::m_OnKeyReleased);
		g_gameWindow->OnResized.Add(this, &Application::m_OnWindowResized);

		if(startFullscreen)
			g_gameWindow->SwitchFullscreen(fullscreenMonitor);

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

	if(m_commandLine.Contains("-test")) 
	{
		// Create test scene
		g_tickables.Add(Test::Create());
	}
	else
	{
		// Play the map
		if(!LaunchMap(m_commandLine[1]))
		{
			Logf("LaunchMap(%s) failed", Logger::Error, m_commandLine[1]);
			return 1;
		}
	}

	Timer appTimer;
	m_lastRenderTime = 0.0f;
	m_lastUpdateTime = 0.0f;
	while(true)
	{
		static const float maxDeltaTime = (1.0f / 30.0f);


		// Gameplay loop
		/// TODO: Add timing management
		for(uint32 i = 0; i < 1; i++)
		{
			// Input update
			if(!g_gameWindow->Update())
				return 0;
			float currentTime = appTimer.SecondsAsFloat();
			float deltaTime = Math::Min(currentTime - m_lastUpdateTime, maxDeltaTime);
			m_lastUpdateTime = currentTime;

			if(!g_tickables.empty())
			{
				IApplicationTickable* tickable = g_tickables.back();
				tickable->Tick(deltaTime);
			}
		}

		// Set time in render state
		m_renderStateBase.time = m_lastUpdateTime;

		// Render loop
		for(uint32 i = 0; i < 1; i++)
		{
			float currentTime = appTimer.SecondsAsFloat();
			float deltaTime = Math::Min(currentTime - m_lastRenderTime, maxDeltaTime);
			m_lastRenderTime = currentTime;

			// Not minimized / Valid resolution
			if(g_resolution.x > 0 && g_resolution.y > 0)
			{
				if(!g_tickables.empty())
				{
					IApplicationTickable* tickable = g_tickables.back();

					tickable->Render(deltaTime);
					g_gl->SwapBuffers();
				}
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

	for(auto it : g_tickables)
	{
		delete it;
	}
	g_game = nullptr;

	CleanupMap();

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

	CleanupMap();

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
	m_lastMapPath = actualMapPath;

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

	g_tickables.Add(g_game);

	return true;
}
void Application::Shutdown()
{
	g_gameWindow->Close();
}

void Application::CleanupMap()
{
	if(m_currentMap)
	{
		delete m_currentMap;
		m_currentMap = nullptr;
	}
}
void Application::CleanupGame()
{
	if(g_game)
	{
		g_tickables.Remove(g_game);
		delete g_game;
		g_game = nullptr;
	}
	CleanupMap();
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
	String pathG = String("shaders/") + name + ".gs";
	Material ret = MaterialRes::Create(g_gl, pathV, pathF);
	// Additionally load geometry shader
	if(Path::FileExists(pathG))
	{
		Shader gshader = ShaderRes::Create(g_gl, ShaderType::Geometry, pathG);
		assert(gshader);
		ret->AssignShader(ShaderType::Geometry, gshader);
	}
	assert(ret);
	return ret;
}
Sample Application::LoadSample(const String& name)
{
	String path = String("audio/") + name + ".wav";
	Sample ret = g_audio->CreateSample(path);
	assert(ret);
	return ret;
}
Transform Application::GetGUIProjection() const
{
	return ProjectionMatrix::CreateOrthographic(0.0f, (float)g_resolution.x, (float)g_resolution.y, 0.0f, 0.0f, 100.0f);
}
void Application::m_OnKeyPressed(Key key)
{
	if(g_game)
	{
		g_game->OnKeyPressed(key);
	}
	if(key == Key::Escape)
	{
		Shutdown();
	}
	if(key == Key::F5) // Restart map
	{
		CleanupGame();
		LaunchMap(m_lastMapPath);
	}
}
void Application::m_OnKeyReleased(Key key)
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
	glViewport(0, 0, newSize.x, newSize.y);
	glScissor(0, 0, newSize.x, newSize.y);
}