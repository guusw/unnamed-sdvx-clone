#include "stdafx.h"
#include "SongSelect.hpp"
#include "Application.hpp"
#include "Profiling.hpp"
#include "Scoring.hpp"
#include "Audio.hpp"
#include "GUI.hpp"
#include "GUI/SongSelectItem.hpp"
#include "MapDatabase.hpp"

#include "Audio.hpp"

class PreviewPlayer
{
public:
	void FadeTo(AudioStream stream)
	{
		// Already existing transition?
		if(m_nextStream)
		{
			if(m_currentStream)
			{
				m_currentStream.Destroy();
			}
			m_currentStream = m_nextStream;
		}
		m_nextStream = stream;
		m_nextStream->SetVolume(0.0f);
		m_nextStream->Play();
		m_fadeTimer = 0.0f;
	}
	void Update(float deltaTime)
	{
		if(m_nextStream)
		{
			m_fadeTimer += deltaTime;
			if(m_fadeTimer >= m_fadeDuration)
			{
				if(m_currentStream)
				{
					m_currentStream.Destroy();
				}
				m_currentStream = m_nextStream;
				m_currentStream->SetVolume(1.0f);
				m_nextStream.Release();
			}
			else
			{
				float fade = m_fadeTimer / m_fadeDuration;

				if(m_currentStream)
					m_currentStream->SetVolume(1.0f - fade);
				m_nextStream->SetVolume(fade);
			}
		}
	}
	void Pause()
	{
		if(m_nextStream)
			m_nextStream->Pause();
		if(m_currentStream)
			m_currentStream->Pause();
	}
	void Restore()
	{
		if(m_nextStream)
			m_nextStream->Play();
		if(m_currentStream)
			m_currentStream->Play();
	}

private:
	static const float m_fadeDuration;
	float m_fadeTimer = 0.0f;
	AudioStream m_nextStream;
	AudioStream m_currentStream;
};
const float PreviewPlayer::m_fadeDuration = 0.5f;

class SongSelect_Impl : public SongSelect
{
private:
	Timer m_dbUpdateTimer;
	Ref<Canvas> m_canvas;
	GUIRenderer m_guiRenderer;
	MapDatabase m_mapDatabase;

	Ref<SongSelectStyle> m_style;

	// Where the window splits between song select and statistics
	static const float m_windowSplit;
	Ref<SongStatistics> m_statisticsWindow;

	Vector<MapIndex*> m_mapRange;
	Map<MapIndex*, Ref<SongSelectItem>> m_guiElements;
	Map<int32, MapIndex*> m_maps;
	int32 m_currentlySelectedId = 0;
	Ref<SongSelectItem> m_currentSelection;

	int32 m_currentlySelectedDiff = 0;

	// Player of preview music
	PreviewPlayer m_previewPlayer;

	// Select sound
	Sample m_selectSound;

public:
	bool Init()
	{
		if(!m_guiRenderer.Init(g_gl))
			return false;

		m_canvas = Utility::MakeRef(new Canvas());

		// Load textures for song select
		m_style = Ref<SongSelectStyle>(new SongSelectStyle);
		m_style->frameSub = g_application->LoadTexture("song_select/sub.png");
		m_style->frameSub->SetWrap(TextureWrap::Clamp, TextureWrap::Clamp);
		m_style->frameMain = g_application->LoadTexture("song_select/main.png");
		m_style->frameMain->SetWrap(TextureWrap::Clamp, TextureWrap::Clamp);
		static const char* diffTextures[] =
		{
			"song_select/nov.png",
			"song_select/adv.png",
			"song_select/exh.png",
			"song_select/grv.png",
			"song_select/inf.png",
		};
		for(uint32 i = 0; i < 5; i++)
		{
			m_style->diffFrames[i] = g_application->LoadTexture(diffTextures[i]);
			m_style->diffFrames[i]->SetWrap(TextureWrap::Clamp, TextureWrap::Clamp);
		}
		m_style->diffFrameMaterial = g_application->LoadMaterial("diffFrame");
		m_style->diffFrameMaterial->opaque = false;

		m_statisticsWindow = Ref<SongStatistics>(new SongStatistics(m_style));
		Canvas::Slot* statisticsSlot = m_canvas->Add(m_statisticsWindow.As<GUIElementBase>());
		statisticsSlot->anchor = Anchor(0, 0, m_windowSplit, 1.0f);

		// Select interface sound
		m_selectSound = g_audio->CreateSample("audio/menu_click.wav");

		// Setup the map database
		if(g_mainConfig.Contains("songfolder"))
		{
			m_mapDatabase.AddSearchPath(g_mainConfig.Get("songfolder")->ToString());
		}
		m_mapDatabase.OnMapsAdded.Add(this, &SongSelect_Impl::OnMapsAdded);
		m_mapDatabase.OnMapsUpdated.Add(this, &SongSelect_Impl::OnMapsUpdated);
		m_mapDatabase.OnMapsRemoved.Add(this, &SongSelect_Impl::OnMapsRemoved);
		m_mapDatabase.OnMapsCleared.Add(this, &SongSelect_Impl::OnMapsCleared);
		m_mapDatabase.StartSearching();

		return true;
	}
	~SongSelect_Impl()
	{
		m_mapDatabase.OnMapsCleared.RemoveAll(this);
	}
	void OnMapsAdded(Vector<MapIndex*> maps)
	{
		for(auto m : maps)
		{
			m_maps.Add(m->id, m);
		}
		if(!m_currentSelection)
			AdvanceSelection(0);
	}
	void OnMapsRemoved(Vector<MapIndex*> maps)
	{
		for(auto m : maps)
		{
			m_maps.erase(m->id);
			auto it = m_guiElements.find(m);
			if(it != m_guiElements.end())
			{
				// Clear selection if a removed item was selected
				if(m_currentSelection == it->second)
					m_currentSelection.Release();

				// Remove this item from the canvas that displays the items
				m_canvas->Remove(it->second.As<GUIElementBase>());
				m_guiElements.erase(it);
			}
		}
		if(!m_maps.Contains(m_currentlySelectedId))
		{
			AdvanceSelection(1);
		}
	}
	void OnMapsUpdated(Vector<MapIndex*> maps)
	{
		for(auto m : maps)
		{
			auto it = m_guiElements.find(m);
			if(it != m_guiElements.end())
			{
				it->second->SetMap(m);
			}
		}
	}
	void OnMapsCleared(Map<int32, MapIndex*> newList)
	{
		m_currentSelection.Release();
		for(auto g : m_guiElements)
		{
			m_canvas->Remove(g.second.As<GUIElementBase>());
		}
		m_guiElements.clear();
		m_maps.clear();
		m_maps = newList;
		if(m_maps.size() > 0)
		{
			AdvanceSelection(0);
		}
	}
	void SelectMap(int32 newIndex)
	{
		Set<MapIndex*> visibleMaps;
		auto it = m_maps.find(newIndex);
		if(it != m_maps.end())
		{
			const float initialSpacing = 0.65f * m_style->frameMain->GetSize().y;
			const float spacing = 0.8f * m_style->frameSub->GetSize().y;
			const Anchor anchor = Anchor(m_windowSplit, 0.5f, 1.0f, 0.5f);

			static const int32 numItems = 10;

			int32 istart;
			for(istart = 0; istart > -numItems;)
			{
				if(it == m_maps.begin())
					break;
				it--;
				istart--;
			}

			for(int32 i = istart; i <= numItems; i++)
			{
				if(it != m_maps.end())
				{
					visibleMaps.Add(it->second);

					// Add a new map slot
					Ref<SongSelectItem> item = GetMapGUIElement(it->second);
					float offset = 0;
					if(i != 0)
					{
						offset = initialSpacing * Math::Sign(i) +
							spacing * (i - Math::Sign(i));
					}
					Canvas::Slot* slot = m_canvas->Add(item.As<GUIElementBase>());

					int32 z = -abs(i);
					slot->SetZOrder(z);

					slot->anchor = anchor;
					slot->autoSizeX = true;
					slot->autoSizeY = true;
					slot->alignment = Vector2(0, 0.5f);
					item->AddAnimation(Ref<IGUIAnimation>(
						new GUIAnimation<Vector2>(&slot->offset.pos, Vector2(0, offset), 0.1f)), true);
					item->AddAnimation(Ref<IGUIAnimation>(
						new GUIAnimation<float>(&slot->offset.size.x, z * 50.0f, 0.1f)), true);

					item->fade = 1.0f - ((float)abs(i) / (float)numItems);
					item->innerOffset = item->fade * 100.0f;

					if(i == 0)
					{
						OnMapSelected(it->second);
					}

					it++;
				}
			}
		}
		m_currentlySelectedId = newIndex;

		// Cleanup invisible elements
		for(auto it = m_guiElements.begin(); it != m_guiElements.end();)
		{
			if(!visibleMaps.Contains(it->first))
			{
				m_canvas->Remove(it->second.As<GUIElementBase>());
				it = m_guiElements.erase(it);
				continue;
			}
			it++;
		}
	}
	void OnMapSelected(MapIndex* map)
	{
		// Update compact mode selection views
		if(m_currentSelection)
			m_currentSelection->SwitchCompact(true);
		m_currentSelection = m_guiElements[map];
		m_currentSelection->SwitchCompact(false);

		// Set current preview audio
		DifficultyIndex* previewDiff = map->difficulties[0];
		String audioPath = map->path + "\\" + previewDiff->settings.audioNoFX;

		AudioStream previewAudio = g_audio->CreateStream(audioPath);
		if(previewAudio)
		{
			previewAudio->SetPosition(previewDiff->settings.previewOffset);
			m_previewPlayer.FadeTo(previewAudio);
		}
		else
		{
			Logf("Failed to load preview audio from [%s]", Logger::Warning, audioPath);
		}

		// Clamp diff selection
		int32 selectDiff = m_currentlySelectedDiff;
		if(m_currentlySelectedDiff >= map->difficulties.size())
		{
			selectDiff = (int32)map->difficulties.size() - 1;
		}
		SelectDifficulty(selectDiff);
	}
	void AdvanceSelection(int32 offset)
	{
		auto it = m_maps.find(m_currentlySelectedId);
		if(it == m_maps.end())
		{
			if(m_maps.empty())
				return;
			it = m_maps.begin();
		}
		for(uint32 i = 0; i < (uint32)abs(offset); i++)
		{
			auto itn = it;
			if(offset < 0)
			{
				if(itn == m_maps.begin())
					break;
				itn--;
			}
			else
				itn++;
			if(itn == m_maps.end())
				break;
			it = itn;
		}
		if(it != m_maps.end())
		{
			SelectMap(it->first);
		}
	}
	void SelectDifficulty(int32 newDiff)
	{
		m_currentSelection->SetSelectedDifficulty(newDiff);
		m_currentlySelectedDiff = newDiff;
		m_selectSound->Play();
	}
	void AdvanceDifficultySelection(int32 offset)
	{
		if(!m_currentSelection)
			return;
		MapIndex* map = m_maps[m_currentlySelectedId];
		int32 newIdx = m_currentlySelectedDiff + offset;
		newIdx = Math::Clamp(newIdx, 0, (int32)map->difficulties.size() - 1);
		SelectDifficulty(newIdx);
	}
	Ref<SongSelectItem> GetMapGUIElement(MapIndex* index)
	{
		auto it = m_guiElements.find(index);
		if(it != m_guiElements.end())
			return it->second;

		Ref<SongSelectItem> newItem = Ref<SongSelectItem>(new SongSelectItem(m_style));

		// Send first map as metadata settings
		const BeatmapSettings& firstSettings = index->difficulties[0]->settings;
		newItem->SetMap(index);
		m_guiElements.Add(index, newItem);
		return newItem;
	}

	virtual void OnKeyPressed(Key key)
	{
		if(key == Key::ArrowDown)
		{
			AdvanceSelection(1);
		}
		else if(key == Key::ArrowUp)
		{
			AdvanceSelection(-1);
		}
		else if(key == Key::PageDown)
		{
			AdvanceSelection(5);
		}
		else if(key == Key::PageUp)
		{
			AdvanceSelection(-5);
		}
		else if(key == Key::ArrowLeft)
		{
			AdvanceDifficultySelection(-1);
		}
		else if(key == Key::ArrowRight)
		{
			AdvanceDifficultySelection(1);
		}
		else if(key == Key::Return)
		{
			if(m_currentSelection)
			{
				MapIndex* map = m_maps[m_currentlySelectedId];
				DifficultyIndex* diff = map->difficulties[m_currentlySelectedDiff];
				if(g_application->LaunchMap(diff->path))
				{
				}
			}
		}
		else if(key == Key::F5)
		{
			m_mapDatabase.StartSearching();
		}
	}
	virtual void OnKeyReleased(Key key)
	{

	}
	virtual void Render(float deltaTime) override
	{
		if(m_dbUpdateTimer.Milliseconds() > 500)
		{
			m_mapDatabase.Update();
			m_dbUpdateTimer.Restart();
		}

		Rect viewport(Vector2(), g_gameWindow->GetWindowSize());
		m_guiRenderer.Render(deltaTime, viewport, m_canvas.As<GUIElementBase>());
	}
	virtual void Tick(float deltaTime) override
	{
		m_previewPlayer.Update(deltaTime);
	}
	virtual void OnSuspend()
	{
		m_previewPlayer.Pause();
		m_mapDatabase.StopSearching();
	}
	virtual void OnRestore()
	{
		m_previewPlayer.Restore();
		m_mapDatabase.StartSearching();
	}
};
const float SongSelect_Impl::m_windowSplit = 0.4f;

SongSelect* SongSelect::Create()
{
	SongSelect_Impl* impl = new SongSelect_Impl();
	if(!impl->Init())
	{
		delete impl;
		return nullptr;
	}
	return impl;
}
