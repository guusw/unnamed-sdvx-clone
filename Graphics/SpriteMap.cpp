#include "stdafx.h"
#include "Image.hpp"
#include "Texture.hpp"
#include <Graphics/ResourceManager.hpp>
#include <set>

static uint32 maxHeight = 1024;
static uint32 border = 2;

struct Category
{
	uint32 width = 0;
	uint32 xOffset = 0;
	uint32 yOffset = 0;
	Vector<uint32> segments;
};
struct Segment
{
	Image image;
	Recti coords;
	Segment(Image pImage) : image(pImage)
	{
	}
	~Segment()
	{
	}
};

class SpriteMap_Impl : public SpriteMapRes
{
	Image m_image;

	Vector<Segment*> m_segments;
	Vector<Category> m_widths;
	Map<uint32, uint32> m_categoryByWidth;
public:
	SpriteMap_Impl()
	{
		m_image = ImageRes::Create();
	}
	~SpriteMap_Impl()
	{
		Clear();
	}

	virtual void SetSize(Vector2i size)
	{
	}
	virtual Vector2i GetSize() const
	{
		return m_image->GetSize();
	}
	virtual Colori* GetBits()
	{
		return m_image->GetBits();
	}
	virtual const Colori* GetBits() const
	{
		return m_image->GetBits();
	}

	Category& GetCategory(uint32 nWidth)
	{
		auto it = m_categoryByWidth.find(nWidth);
		if (it == m_categoryByWidth.end())
		{
			uint32 offset = 0;
			if (!m_widths.empty())
				offset = m_widths.back().xOffset + m_widths.back().width + border;
			m_categoryByWidth.Add(nWidth, (uint32)m_widths.size());
			m_widths.emplace_back();
			m_widths.back().width = nWidth;
			m_widths.back().xOffset = offset;
			return m_widths.back();
		}

		return m_widths[it->second];
	}
	virtual uint32 AddSegment(Image image)
	{

		uint32 nI = (uint32)m_segments.size();
		m_segments.push_back(new Segment(image));
		Segment* pCurrentSegment = m_segments.back();
		pCurrentSegment->coords.size = image->GetSize();

		Category& cat = GetCategory(image->GetSize().x);
		Segment* pLastSegment = cat.segments.empty() ? nullptr : m_segments[cat.segments.back()];

		pCurrentSegment->coords.pos.x = cat.xOffset;
		if (pLastSegment)
			pCurrentSegment->coords.pos.y = cat.yOffset;
		cat.yOffset = pCurrentSegment->coords.Bottom() + border;

		cat.segments.push_back(nI);
		return nI;
	}
	virtual void Clear()
	{
		for (auto s : m_segments)
		{
			delete s;
		}
		m_segments.clear();
		m_widths.clear();
		m_categoryByWidth.clear();
	}
	void CopySubImage(Segment* seg)
	{
		Vector2i size = seg->image->GetSize();
		Colori* pSrc = seg->image->GetBits();
		uint32 nDstPitch = GetSize().x;
		Colori* pDst = GetBits() + seg->coords.pos.x + seg->coords.pos.y * nDstPitch;
		for (uint32 y = 0; y < (uint32)size.y; y++)
		{
			for (uint32 x = 0; x < (uint32)size.x; x++)
			{
				*pDst = *pSrc;
				pSrc++;
				pDst++;
			}
			pDst += (nDstPitch - size.x);
		}
	}

	uint32 GetTotalHeight(uint32 nWidthCategory)
	{
		uint32 nHeight = 0;
		Category& c = GetCategory(nWidthCategory);
		for (auto it = c.segments.begin(); it != c.segments.end(); it++)
		{
			nHeight += m_segments[*it]->coords.size.y + border;
		}
		return nHeight;
	}
	virtual Image Generate()
	{
		// Get Total Max Width
		uint32 nTotalWidth = 0;
		uint32 nTotalHeight = 0;

		for (auto w : m_widths)
		{
			nTotalWidth = w.xOffset + w.width;
			nTotalHeight = max(nTotalHeight, w.yOffset);
		}

		// Round to powers of 
		nTotalWidth = (uint32)pow(2, ceil(log(nTotalWidth) / log(2)));
		nTotalHeight = (uint32)pow(2, ceil(log(nTotalHeight) / log(2)));

		m_image->SetSize(Vector2i(nTotalWidth, nTotalHeight));

		uint32 nXOffset = 0;
		uint32 nYOffset = 0;
		for (auto w : m_widths)
		{
			for (auto sid : w.segments)
			{
				Segment* seg = m_segments[sid];
				CopySubImage(seg);
			}
		}

		return m_image;
	}
	virtual Texture GenerateTexture(OpenGL* gl)
	{
		Image tmp = Generate();
		Texture tex = TextureRes::Create(gl, tmp);
		return tex;
	}
	virtual Recti GetCoords(uint32 nIndex)
	{
		assert(nIndex < m_segments.size());
		return m_segments[nIndex]->coords;
	}
};

SpriteMap SpriteMapRes::Create()
{
	SpriteMap_Impl* pImpl = new SpriteMap_Impl();
	return GetResourceManager<ResourceType::SpriteMap>().Register(pImpl);
}