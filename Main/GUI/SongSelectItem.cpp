#include "stdafx.h"
#include "SongSelectItem.hpp"
#include "GUI.hpp"
#include "Application.hpp"

#include "Beatmap.hpp"
#include "MapDatabase.hpp"

static float padding = 5.0f;

/* A frame that displays the jacket+frame of a single map difficulty */
class SongDifficultyFrame : public GUIElementBase
{
private:
	Ref<SongSelectStyle> m_style;
	static const Vector2 m_size;
	DifficultyIndex* m_diff;
	Texture m_jacket;
	Texture m_frame;
	Text m_lvlText;

	bool m_selected = false;
	float m_fade = 0.0f;
public:
	SongDifficultyFrame(Ref<SongSelectStyle> style, DifficultyIndex* diff)
	{
		m_style = style;
		m_diff = diff;
		m_frame = m_style->diffFrames[Math::Min<size_t>(diff->settings.difficulty, m_style->numDiffFrames-1)];
	}
	virtual void Render(GUIRenderData rd)
	{
		m_TickAnimations(rd.deltaTime);

		// Load jacket?
		if(!m_jacket)
		{
			String jacketPath = m_diff->path;
			jacketPath = Path::Normalize(Path::RemoveLast(jacketPath) + "//" + m_diff->settings.jacketPath);
			m_jacket = m_style->GetJacketThumnail(jacketPath);
		}

		// Render lvl text?
		if(!m_lvlText)
		{
			WString lvlStr = Utility::WSprintf(L"%d", m_diff->settings.level);
			m_lvlText = rd.guiRenderer->font->CreateText(lvlStr, 20);
		}

		Rect area = GUISlotBase::ApplyFill(FillMode::Fit, m_size, rd.area);
		static const float scale = 0.1f;
		area.pos -= area.size * scale * m_fade * 0.5f;
		area.size += area.size * scale * m_fade;
		rd.guiRenderer->SetScissorRect(area);

		// Custom rendering to combine jacket image and frame
		Transform transform;
		transform *= Transform::Translation(area.pos);
		transform *= Transform::Scale(Vector3(area.size.x, area.size.y, 1.0f));
		MaterialParameterSet params;
		params.SetParameter("selected", m_selected ? 1.0f : 0.0f);
		params.SetParameter("frame", m_frame);
		if(m_jacket)
			params.SetParameter("jacket", m_jacket);
		rd.rq->DrawScissored(area, transform, rd.guiRenderer->guiQuad, m_style->diffFrameMaterial, params);

		// Render level text
		Rect textRect = Rect(Vector2(), m_lvlText->size);
		Rect textFrameRect = textRect;
		textFrameRect.size.x = area.size.x * 0.25f;
		textFrameRect = GUISlotBase::ApplyAlignment(Vector2(1, 0), textFrameRect, area);
		textRect = GUISlotBase::ApplyAlignment(Vector2(0.5f, 0.5f), textRect, textFrameRect);
		rd.guiRenderer->RenderRect(*rd.rq, textFrameRect, Color::Black.WithAlpha(0.5f));
		rd.guiRenderer->RenderText(*rd.rq, m_lvlText, textRect.pos, Color::White);
	}
	virtual bool GetDesiredSize(GUIRenderData rd, Vector2& sizeOut)
	{
		Rect base = GUISlotBase::ApplyFill(FillMode::Fit, m_size, rd.area);
		sizeOut = base.size;
		return true;
	}
	virtual void SetSelected(bool selected)
	{
		if(m_selected != selected)
		{
			m_selected = selected;
			// Zoom in animation
			AddAnimation(Ref<IGUIAnimation>(
				new GUIAnimation<float>(&m_fade, selected ? 1.0f : 0.0f, 0.2f)), true);
		}
	}

};
const Vector2 SongDifficultyFrame::m_size = Vector2(512, 512);

SongSelectItem::SongSelectItem(Ref<SongSelectStyle> style)
{
	m_style = style;

	// Background image
	{
		m_bg = new Panel();
		m_bg->imageFillMode = FillMode::None;
		m_bg->imageAlignment = Vector2(1.0f, 0.5f);
		Slot* bgSlot = Add(m_bg->MakeShared());
		bgSlot->anchor = Anchors::Full;
	}

	// Add Main layout container
	{
		m_mainVert = new LayoutBox();
		m_mainVert->layoutDirection = LayoutBox::Vertical;
		Slot* slot = Add(m_mainVert->MakeShared());
	}

	// Add Titles
	{
		m_title = new Label();
		m_title->SetFontSize(40);
		m_title->SetText(L"FRANK IS DE MEESTER");
		LayoutBox::Slot* slot = m_mainVert->Add(m_title->MakeShared());
		slot->padding = Margin(0, -5.0f);

		m_artist = new Label();
		m_artist->SetFontSize(32);
		m_artist->SetText(L"“ú–{ PIPES");
		slot = m_mainVert->Add(m_artist->MakeShared());
		slot->padding = Margin(0, -5.0f);
	}

	// Add diff select
	{
		m_diffSelect = new LayoutBox();
		m_diffSelect->layoutDirection = LayoutBox::Horizontal;
		Slot* slot = Add(m_diffSelect->MakeShared());
		slot->anchor = Anchor(0.0f, 0.4f, 1.0f, 1.0f - 0.09f);
		slot->padding = Margin(padding, 0, 0, 0);
	}

	SwitchCompact(true);
}
void SongSelectItem::Render(GUIRenderData rd)
{
	// Update fade
	m_title->color.w = fade;
	m_artist->color.w = fade;

	// Update inner offset
	m_mainVert->slot->padding.left = padding + innerOffset;
	m_diffSelect->slot->padding.left = padding + innerOffset;

	// Render canvas
	Canvas::Render(rd);
}

bool SongSelectItem::GetDesiredSize(GUIRenderData rd, Vector2& sizeOut)
{
	sizeOut = m_bg->texture->GetSize();
	sizeOut.x = Math::Min(sizeOut.x, rd.area.size.x);
	return true;
}
void SongSelectItem::SetMap(struct MapIndex* map)
{
	const BeatmapSettings& settings = map->difficulties[0]->settings;
	m_title->SetText(Utility::ConvertToWString(settings.title));
	m_artist->SetText(Utility::ConvertToWString(settings.artist));

	// Add all difficulty icons
	m_diffSelect->Clear();
	m_diffSelectors.clear();
	for(auto d : map->difficulties)
	{
		SongDifficultyFrame* frame = new SongDifficultyFrame(m_style, d);
		LayoutBox::Slot* slot = m_diffSelect->Add(frame->MakeShared());
		slot->padding = Margin(2);
		m_diffSelectors.Add(frame);
	}
}
void SongSelectItem::SwitchCompact(bool compact)
{
	Slot* mainSlot = (Slot*)m_mainVert->slot;
	Slot* bgSlot = (Slot*)m_bg;
	if(compact)
	{
		m_bg->texture = m_style->frameSub;
		m_bg->texture->SetWrap(TextureWrap::Clamp, TextureWrap::Clamp);
		m_diffSelect->visibility = Visibility::Collapsed;

		mainSlot->anchor = Anchor(0.0f, 0.5f, 1.0, 0.5f);
		mainSlot->autoSizeX = true;
		mainSlot->autoSizeY = true;
		mainSlot->alignment = Vector2(0.0f, 0.5f); 
	}
	else
	{
		m_bg->texture = m_style->frameMain; 
		m_bg->texture->SetWrap(TextureWrap::Clamp, TextureWrap::Clamp);
		m_diffSelect->visibility = Visibility::Visible;

		// Take 0.3 of top 
		mainSlot->anchor = Anchor(0.0f, 0.09f, 1.0, 0.3f);
		mainSlot->autoSizeX = true;
		mainSlot->autoSizeY = true;
		mainSlot->alignment = Vector2(0.0f, 0.5f);
	}
}
void SongSelectItem::SetSelectedDifficulty(int32 selectedIndex)
{
	if(selectedIndex < 0)
		return;
	if(selectedIndex < m_diffSelectors.size())
	{
		if(m_selectedDifficulty < m_diffSelectors.size())
		{
			m_diffSelectors[m_selectedDifficulty]->SetSelected(false);
		}
		m_diffSelectors[selectedIndex]->SetSelected(true);
		m_selectedDifficulty = selectedIndex;
	}
}

Texture SongSelectStyle::GetJacketThumnail(const String& path)
{
	auto it = m_jacketImages.find(path);
	if(it == m_jacketImages.end())
	{
		CachedImage newImage;
		newImage.lastUsage = m_timer.SecondsAsFloat();
		Image img = ImageRes::Create(path);
		if(!img)
		{
			m_jacketImages.Add(path, newImage);
		}
		else
		{
			newImage.texture = TextureRes::Create(g_gl, img);
			newImage.texture->SetWrap(TextureWrap::Clamp, TextureWrap::Clamp);
		}
		m_jacketImages.Add(path, newImage);
		return newImage.texture;
	}
	else
	{
		it->second.lastUsage = m_timer.SecondsAsFloat();
		return it->second.texture;
	}
}

SongStatistics::SongStatistics(Ref<SongSelectStyle> style)
{
	m_style = style;

	m_bg = new Panel();
	m_bg->color = Color::White.WithAlpha(0.5f);
	Slot* slot = Add(m_bg->MakeShared());
	slot->anchor = Anchors::Full;
	slot->SetZOrder(-1);
}
