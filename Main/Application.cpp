#include "stdafx.h"
#include "Application.hpp"
#include "Beatmap.hpp"
#include "Game.hpp"
#include "Test.hpp"
#include "SongSelect.hpp"
#include "Audio.hpp"
#include <Graphics/Window.hpp>
#include <Graphics/ResourceManagers.hpp>
#include "Profiling.hpp"
#include "Scoring.hpp"

Config g_mainConfig;
OpenGL* g_gl = nullptr;
Window* g_gameWindow = nullptr;
Application* g_application = nullptr;

Game* g_game = nullptr;

// Tickable queue
static Vector<IApplicationTickable*> g_tickables;
static Vector<IApplicationTickable*> g_removalQueue;

// Used to set the initial screen size
static float g_screenHeight = 1000.0f;

// Current screen size
float g_aspectRatio = (16.0f / 9.0f);
Vector2i g_resolution;

// Render FPS cap
static int32 g_fpsCap = 60;
static float g_targetRenderTime = 0.1f;
// Update target FPS
static float g_targetUpdateTime = 1.0f / 240.0f;

static float g_avgUpdateDelta = 0.0f;
static float g_avgRenderDelta = 0.0f;

Application::Application()
{
	// Enforce single instance
	assert(!g_application);
	g_application = this;

	// Init FPS cap
	SetFrameLimiter(g_fpsCap);
}
Application::~Application()
{
	m_Cleanup();
	assert(g_application == this);
	g_application = nullptr;
}
int32 Application::Run()
{
	if(!m_Init())
		return 1;

	if(m_commandLine.Contains("-test")) 
	{
		// Create test scene
		g_tickables.Add(Test::Create());
	}
	else
	{
		bool mapLaunched = false;
		// Play the map specified in the command line
		if(m_commandLine.size() > 1)
		{
			if(!LaunchMap(m_commandLine[1]))
			{
				Logf("LaunchMap(%s) failed", Logger::Error, m_commandLine[1]);
			}
			else
			{
				if(g_application->GetAppCommandLine().Contains("-autoplay"))
				{
					g_game->GetScoring().autoplay = true;
				}
				mapLaunched = true;
			}
		}

		if(!mapLaunched)
		{
			// Start regular game, goto song select
			g_tickables.Add(SongSelect::Create());
		}
	}

	m_MainLoop();

	return 0;
}

bool Application::m_LoadConfig()
{
	File configFile;
	if(configFile.OpenRead("Main.cfg"))
	{
		FileReader reader(configFile);
		if(g_mainConfig.Load(reader))
			return true;
	}
	return false;
}
void Application::m_LoadDefaultConfig()
{
	g_mainConfig.Clear();
	g_mainConfig.Add("songfolder", Variant::Create("songs"));
}
void Application::m_SaveConfig()
{
	File configFile;
	if(configFile.OpenWrite("Main.cfg"))
	{
		FileWriter writer(configFile);
		g_mainConfig.Save(writer);
	}
}

bool Application::m_Init()
{
	ProfilerScope $("Application Setup");

	// Split up command line parameters
	String cmdLine = Utility::ConvertToUTF8(GetCommandLine());
	m_commandLine = Path::SplitCommandLine(cmdLine);
	assert(m_commandLine.size() >= 1);

	if(!m_LoadConfig())
		m_LoadDefaultConfig();

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
		// Test tracks may get annoying when continously debugging ;)
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
			return false;
		}
	}
	return true;
}
void Application::m_MainLoop()
{
	Timer dumpFrameRate;
	Timer appTimer;
	m_lastRenderTime = 0.0f;
	m_lastUpdateTime = 0.0f;
	while(true)
	{
		// Gameplay loop
		float currentTime = appTimer.SecondsAsFloat();
		float timeSinceUpdate = currentTime - m_lastUpdateTime;
		if(timeSinceUpdate > 1.0f) // Should only happen when game freezes / Debugger paused
		{
			timeSinceUpdate = g_targetUpdateTime;
			m_lastUpdateTime = currentTime - g_targetUpdateTime;
		}
		while(timeSinceUpdate >= g_targetUpdateTime)
		{
			// Input update
			if(!g_gameWindow->Update())
				return;

			// Calculate actual deltatime for timing calculations
			currentTime = appTimer.SecondsAsFloat();
			float actualDeltaTime = currentTime - m_lastUpdateTime;
			g_avgUpdateDelta = g_avgUpdateDelta * 0.99f + actualDeltaTime * 0.01f; // Calculate avg
			m_lastUpdateTime = currentTime;

			// Fixed DeltaTime
			float deltaTime = g_targetUpdateTime;
			timeSinceUpdate -= g_targetUpdateTime;

			// Remove remove-queued tickables
			bool removed = false;
			for(auto it = g_tickables.begin(); it != g_tickables.end();)
			{
				if(g_removalQueue.Contains(*it))
				{
					delete *it;
					if(*it == g_game)
					{
						// Game removed
						g_game = nullptr;
					}
					it = g_tickables.erase(it);
					removed = true;
					continue;
				}
				it++;
			}
			g_removalQueue.clear();

			if(!g_tickables.empty())
			{
				IApplicationTickable* tickable = g_tickables.back();
				if(removed)
					tickable->OnRestore();
				tickable->Tick(deltaTime);
			}
			else
			{
				// Shutdown when all menus are gone
				Logf("No more application windows, shutting down", Logger::Warning);
				Shutdown();
			}
		}

		// Render loop
		currentTime = appTimer.SecondsAsFloat();
		float timeSinceRender = currentTime - m_lastRenderTime;
		if(timeSinceRender > g_targetRenderTime)
		{
			// Also update window in render loop
			if(!g_gameWindow->Update())
				return;

			// Calculate actual deltatime for timing calculations
			currentTime = appTimer.SecondsAsFloat();
			float actualDeltaTime = currentTime - m_lastRenderTime;
			g_avgRenderDelta = g_avgRenderDelta * 0.99f + actualDeltaTime * 0.01f; // Calculate avg

			// Fixed DeltaTime
			float deltaTime = g_targetRenderTime;
			if(g_fpsCap <= 0)
				deltaTime = actualDeltaTime;
			m_lastRenderTime = currentTime;

			// Set time in render state
			m_renderStateBase.time = currentTime;

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

	// Finally, save config
	m_SaveConfig();
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
	{
		Logf("Failed to load map", Logger::Warning);
		return false;
	}

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
	String pathNormalized = Path::Normalize(actualMapPath);
	String mapBasePath = Path::RemoveLast(pathNormalized);

	g_game = Game::Create(m_currentMap, mapBasePath);
	if(!g_game)
		return false;

	AddTickable(g_game);

	return true;
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
		RemoveTickable(g_game);
		g_game = nullptr;
	}
}
void Application::Shutdown()
{
	g_gameWindow->Close();
}

void Application::AddTickable(class IApplicationTickable* tickable)
{
	if(!g_tickables.empty())
		g_tickables.back()->OnSuspend();
	g_tickables.Add(tickable);
}
void Application::RemoveTickable(IApplicationTickable* tickable)
{
	if(g_tickables.Contains(tickable))
	{
		g_removalQueue.AddUnique(tickable);
	}
}

String Application::GetCurrentMapPath()
{
	return m_lastMapPath;
}

const Vector<String>& Application::GetAppCommandLine() const
{
	return m_commandLine;
}
RenderState Application::GetRenderStateBase() const
{
	return m_renderStateBase;
}

void Application::SetFrameLimiter(int32 fpsCap)
{
	g_fpsCap = fpsCap;
	if(fpsCap <= 0)
		g_targetRenderTime = 0.0f;
	else
		g_targetRenderTime = 1.0f / (float)fpsCap;
	Logf("FPS cap set to %d", Logger::Info, fpsCap);
}

Texture Application::LoadTexture(const String& name)
{
	String path = String("textures/") + name;
	Image img = ImageRes::Create(path);
	Texture ret = TextureRes::Create(g_gl, img);
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

float Application::GetUpdateFPS() const
{
	return 1.0f / g_avgUpdateDelta;
}
float Application::GetRenderFPS() const
{
	return 1.0f / g_avgRenderDelta;
}

Transform Application::GetGUIProjection() const
{
	return ProjectionMatrix::CreateOrthographic(0.0f, (float)g_resolution.x, (float)g_resolution.y, 0.0f, 0.0f, 100.0f);
}
void Application::m_OnKeyPressed(Key key)
{
	// Fullscreen toggle
	if(key == Key::Return)
	{
		if((g_gameWindow->GetModifierKeys() & ModifierKeys::Alt) == ModifierKeys::Alt)
		{
			g_gameWindow->SwitchFullscreen();
			return;
		}
	}

	// Pass key to application
	for(auto it = g_tickables.rbegin(); it != g_tickables.rend();)
	{
		(*it)->OnKeyPressed(key);
		break;
	}
}
void Application::m_OnKeyReleased(Key key)
{
	for(auto it = g_tickables.rbegin(); it != g_tickables.rend();)
	{
		(*it)->OnKeyReleased(key);
		break;
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