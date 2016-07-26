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

/* 
	Song preview player with fade-in/out 
*/
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

/*
	Song selection wheel
*/
class SelectionWheel : public Canvas
{
	Map<MapIndex*, Ref<SongSelectItem>> m_guiElements;	

	Vector<MapIndex*> m_mapRange;
	Map<int32, MapIndex*> m_maps;

	Map<int32, MapIndex*> m_mapFilter;
	bool m_filterSet = false;

	// Currently selected map ID
	int32 m_currentlySelectedId = 0;
	// Currently selected sub-widget
	Ref<SongSelectItem> m_currentSelection;

	// Current difficulty index
	int32 m_currentlySelectedDiff = 0;

	// Style to use for everything song select related
	Ref<SongSelectStyle> m_style;

public:
	SelectionWheel(Ref<SongSelectStyle> style) : m_style(style)
	{
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
				Remove(it->second.As<GUIElementBase>());
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
			Remove(g.second.As<GUIElementBase>());
		}
		m_guiElements.clear();
		m_filterSet = false;
		m_mapFilter.clear();
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
		auto& srcCollection = m_SourceCollection();
		auto it = srcCollection.find(newIndex);
		if(it != srcCollection.end())
		{
			const float initialSpacing = 0.65f * m_style->frameMain->GetSize().y;
			const float spacing = 0.8f * m_style->frameSub->GetSize().y;
			const Anchor anchor = Anchor(0.0f, 0.5f, 1.0f, 0.5f);

			static const int32 numItems = 10;

			int32 istart;
			for(istart = 0; istart > -numItems;)
			{
				if(it == srcCollection.begin())
					break;
				it--;
				istart--;
			}

			for(int32 i = istart; i <= numItems; i++)
			{
				if(it != srcCollection.end())
				{
					visibleMaps.Add(it->second);

					// Add a new map slot
					Ref<SongSelectItem> item = m_GetMapGUIElement(it->second);
					float offset = 0;
					if(i != 0)
					{
						offset = initialSpacing * Math::Sign(i) +
							spacing * (i - Math::Sign(i));
					}
					Canvas::Slot* slot = Add(item.As<GUIElementBase>());

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
						m_OnMapSelected(it->second);
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
				Remove(it->second.As<GUIElementBase>());
				it = m_guiElements.erase(it);
				continue;
			}
			it++;
		}
	}
	void AdvanceSelection(int32 offset)
	{
		auto& srcCollection = m_SourceCollection();
		auto it = srcCollection.find(m_currentlySelectedId);
		if(it == srcCollection.end())
		{
			if(srcCollection.empty())
			{
				// Remove all elements, empty
				m_currentSelection.Release();
				Clear();
				m_guiElements.clear();
				return;
			}
			it = srcCollection.begin();
		}
		for(uint32 i = 0; i < (uint32)abs(offset); i++)
		{
			auto itn = it;
			if(offset < 0)
			{
				if(itn == srcCollection.begin())
					break;
				itn--;
			}
			else
				itn++;
			if(itn == srcCollection.end())
				break;
			it = itn;
		}
		if(it != srcCollection.end())
		{
			SelectMap(it->first);
		}
	}
	void SelectDifficulty(int32 newDiff)
	{
		m_currentSelection->SetSelectedDifficulty(newDiff);
		m_currentlySelectedDiff = newDiff;

		MapIndex** map = m_maps.Find(m_currentlySelectedId);
		if(map)
		{
			OnDifficultySelected.Call(map[0]->difficulties[m_currentlySelectedDiff]);
		}
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

	// Called when a new map is selected
	Delegate<MapIndex*> OnMapSelected;
	Delegate<DifficultyIndex*> OnDifficultySelected;

	// Set display filter
	void SetFilter(Map<int32, MapIndex *> filter)
	{
		m_mapFilter = filter;
		m_filterSet = true;
		AdvanceSelection(0);
	}
	void ClearFilter()
	{
		if(m_filterSet)
		{
			m_filterSet = false;
			AdvanceSelection(0);
		}
	}

	MapIndex* GetSelection() const
	{
		MapIndex*const* map = m_maps.Find(m_currentlySelectedId);
		if(map)
			return *map;
		return nullptr;
	}
	DifficultyIndex* GetSelectedDifficulty() const
	{
		MapIndex*const* map = m_maps.Find(m_currentlySelectedId);
		if(map)
		{
			return map[0]->difficulties[m_currentlySelectedDiff];
		}
		return nullptr;
	}

private:
	const Map<int32, MapIndex*>& m_SourceCollection()
	{
		return m_filterSet ? m_mapFilter : m_maps;
	}
	Ref<SongSelectItem> m_GetMapGUIElement(MapIndex* index)
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
	void m_OnMapSelected(MapIndex* map)
	{
		// Update compact mode selection views
		if(m_currentSelection)
			m_currentSelection->SwitchCompact(true);
		m_currentSelection = m_guiElements[map];
		m_currentSelection->SwitchCompact(false);

		//if(map && map->id == m_currentlySelectedId)
		//	return;

		// Clamp diff selection
		int32 selectDiff = m_currentlySelectedDiff;
		if(m_currentlySelectedDiff >= (int32)map->difficulties.size())
		{
			selectDiff = (int32)map->difficulties.size() - 1;
		}
		SelectDifficulty(selectDiff);

		OnMapSelected.Call(map);
	}
};

/*
	Song select window/screen
*/
class SongSelect_Impl : public SongSelect
{
private:
	Timer m_dbUpdateTimer;
	Ref<Canvas> m_canvas;
	GUIRenderer m_guiRenderer;
	MapDatabase m_mapDatabase;

	Ref<SongSelectStyle> m_style;

	// Shows additional information about a map
	Ref<SongStatistics> m_statisticsWindow;
	// Map selection wheel
	Ref<SelectionWheel> m_selectionWheel;
	// Search field
	Ref<TextInputField> m_searchField;

	// Player of preview music
	PreviewPlayer m_previewPlayer;

	// Select sound
	Sample m_selectSound;

public:
	bool Init()
	{
		if(!m_guiRenderer.Init(g_gl, g_gameWindow))
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

		// Split between statistics and selection wheel (in percentage)
		const float screenSplit = 0.4f;

		m_statisticsWindow = Ref<SongStatistics>(new SongStatistics(m_style));
		Canvas::Slot* statisticsSlot = m_canvas->Add(m_statisticsWindow.As<GUIElementBase>());
		statisticsSlot->anchor = Anchor(0, 0, screenSplit, 1.0f);

		LayoutBox* box = new LayoutBox();
		box->fill = false;
		Canvas::Slot* boxSlot = m_canvas->Add(box->MakeShared());
		boxSlot->anchor = Anchor(screenSplit, 0, 1.0f, 1.0f);
		box->layoutDirection = LayoutBox::Vertical;
		{
			m_searchField = Ref<TextInputField>(new TextInputField());
			LayoutBox::Slot* searchFieldSlot = box->Add(m_searchField.As<GUIElementBase>());
			searchFieldSlot->fill = false;
			m_guiRenderer.SetInputFocus(m_searchField.GetData());
			m_searchField->OnTextUpdated.Add(this, &SongSelect_Impl::OnSearchTermChanged);

			m_selectionWheel = Ref<SelectionWheel>(new SelectionWheel(m_style));
			LayoutBox::Slot* selectionSlot = box->Add(m_selectionWheel.As<GUIElementBase>());
			selectionSlot->fill = true;
			m_selectionWheel->OnMapSelected.Add(this, &SongSelect_Impl::OnMapSelected);
			m_selectionWheel->OnDifficultySelected.Add(this, &SongSelect_Impl::OnDifficultySelected);
		}


		// Select interface sound
		m_selectSound = g_audio->CreateSample("audio/menu_click.wav");

		// Setup the map database
		if(g_mainConfig.Contains("songfolder"))
		{
			m_mapDatabase.AddSearchPath(g_mainConfig.Get("songfolder")->ToString());
		}

		m_mapDatabase.OnMapsAdded.Add(m_selectionWheel.GetData(), &SelectionWheel::OnMapsAdded);
		m_mapDatabase.OnMapsUpdated.Add(m_selectionWheel.GetData(), &SelectionWheel::OnMapsUpdated);
		m_mapDatabase.OnMapsRemoved.Add(m_selectionWheel.GetData(), &SelectionWheel::OnMapsRemoved);
		m_mapDatabase.OnMapsCleared.Add(m_selectionWheel.GetData(), &SelectionWheel::OnMapsCleared);
		m_mapDatabase.StartSearching();

		return true;
	}
	~SongSelect_Impl()
	{
		// Clear callbacks
		m_mapDatabase.OnMapsCleared.Clear();
	}
	
	// When a map is selected in the song wheel
	void OnMapSelected(MapIndex* map)
	{
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
	}
	// When a difficulty is selected in the song wheel
	void OnDifficultySelected(DifficultyIndex* diff)
	{
	}
	void OnSearchTermChanged(const WString& search)
	{
		if(search.empty())
			m_selectionWheel->ClearFilter();
		else
		{
			String utf8Search = Utility::ConvertToUTF8(search);
			Map<int32, MapIndex*> filter = m_mapDatabase.FindMaps(utf8Search);
			m_selectionWheel->SetFilter(filter);
		}
	}

	virtual void OnKeyPressed(Key key)
	{
		if(key == Key::ArrowDown)
		{
			m_selectionWheel->AdvanceSelection(1);
		}
		else if(key == Key::ArrowUp)
		{
			m_selectionWheel->AdvanceSelection(-1);
		}
		else if(key == Key::PageDown)
		{
			m_selectionWheel->AdvanceSelection(5);
		}
		else if(key == Key::PageUp)
		{
			m_selectionWheel->AdvanceSelection(-5);
		}
		else if(key == Key::ArrowLeft)
		{
			m_selectionWheel->AdvanceDifficultySelection(-1);
		}
		else if(key == Key::ArrowRight)
		{
			m_selectionWheel->AdvanceDifficultySelection(1);
		}
		else if(key == Key::Return)
		{
			MapIndex* map = m_selectionWheel->GetSelection();
			if(map)
			{
				DifficultyIndex* diff = m_selectionWheel->GetSelectedDifficulty();
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

		glClearColor(0, 0, 0, 0);
		glClear(GL_COLOR_BUFFER_BIT);

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
		m_guiRenderer.SetInputFocus(nullptr);
	}
	virtual void OnRestore()
	{
		m_previewPlayer.Restore();
		m_mapDatabase.StartSearching();
		m_guiRenderer.SetInputFocus(m_searchField.GetData());

		OnSearchTermChanged(m_searchField->GetText());
	}
};

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
