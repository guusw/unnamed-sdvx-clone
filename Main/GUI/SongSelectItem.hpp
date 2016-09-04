#pragma once
#include "GUIElement.hpp"
#include "Canvas.hpp"
#include "SongSelectStyle.hpp"

// Song select item
//	either shows only artist and title in compact mode
//	or shows all the difficulties
class SongSelectItem : public Canvas
{
public:
	SongSelectItem(Ref<SongSelectStyle> style);
	virtual void PreRender(GUIRenderData rd, GUIElementBase*& inputElement) override;
	virtual void Render(GUIRenderData rd) override;
	virtual Vector2 GetDesiredSize(GUIRenderData rd) override;

	// Assigns/Updates the map to be displayed on this item
	void SetMap(struct MapIndex* map);

	// Select compact of full display
	void SwitchCompact(bool compact);

	// Set selected difficulty
	void SetSelectedDifficulty(int32 selectedIndex);

	// Fade of text color
	float fade = 1.0f;

	// Offset of inner text/difficulty selector
	float innerOffset = 0.0f;

private:
	Ref<SongSelectStyle> m_style;
	Vector<class SongDifficultyFrame*> m_diffSelectors;
	int32 m_selectedDifficulty = 0;
	class Panel* m_bg = nullptr;
	class LayoutBox* m_mainVert = nullptr;
	class LayoutBox* m_diffSelect = nullptr;
	class Label* m_title = nullptr;
	class Label* m_artist = nullptr;
};

// Song statistics window on the left
class SongStatistics : public Canvas
{
public:
	SongStatistics(Ref<SongSelectStyle> style);

private:
	Ref<SongSelectStyle> m_style;
	class Panel* m_bg = nullptr;
};