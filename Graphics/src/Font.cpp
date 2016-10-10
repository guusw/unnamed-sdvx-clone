#include "stdafx.h"
#include "Font.hpp"
#include "ResourceManagers.hpp"
#include "Image.hpp"
#include "Texture.hpp"
#include "Mesh.hpp"
#include "OpenGL.hpp"
#include <Shared/Timer.hpp>

#include <ft2build.h>
#include FT_FREETYPE_H

namespace Graphics
{
	struct CachedText
	{
		Text text;
		float lastUsage;
	};

	// Prevents continuous recreation of text that doesn't change
	class TextCache : public Map<WString, CachedText>
	{
		Timer timer;
	public:
		void Update()
		{
			float currentTime = timer.SecondsAsFloat();
			for(auto it = begin(); it != end();)
			{
				float durationSinceUsed = currentTime - it->second.lastUsage;
				if(durationSinceUsed > 1.0f)
				{
					it = erase(it);
					continue;
				}
				it++;
			}
		}
		Text GetText(const WString& key)
		{
			auto it = find(key);
			if(it != end())
			{
				it->second.lastUsage = timer.SecondsAsFloat();
				return it->second.text;
			}
			return Text();
		}
		void AddText(const WString& key, Text obj)
		{
			Update();
			Add(key, { obj, timer.SecondsAsFloat() });
		}
	};

	FT_Library library;
	class Font_Impl;

	using Utility::Reinterpret;
	using VectorMath::VectorBase;

	FT_Face fallbackFont;
	uint32 fallbackFontSize = 0;

	// Initializer handle object for the Freetype library
	struct FontLibrary
	{
		FontLibrary();
		~FontLibrary();
		bool LoadFallbackFont();
		Buffer loadedFallbackFont;
	};
	// Keep freetype statically initialized for application lifetime
	static FontLibrary _libraryInitializer;

	struct CharInfo : public GlyphInfo
	{
		uint32 glyphID;
	};

	// A single font size
	struct FontSize
	{
		SpriteMap spriteMap;
		Texture textureMap;
		FT_Face face;
		Vector<CharInfo> infos;
		Map<wchar_t, uint32> infoByChar;
		bool bUpdated = false;

		// See https://www.freetype.org/freetype2/docs/glyphs/glyphs-3.html
		float ascender;
		float descender;
		float lineHeight; // linegap?

		uint32 size;
		TextCache cache;

		FontSize(OpenGL* gl, FT_Face& face)
			: face(face), m_gl(gl)
		{
			spriteMap = SpriteMapRes::Create();
			textureMap = TextureRes::Create(m_gl);
			ascender = (float)face->size->metrics.ascender / 64.0f;
			descender = (float)face->size->metrics.descender / 64.0f;
			lineHeight = (float)face->size->metrics.height / 64.0f;
		}
		~FontSize()
		{
		}

		const CharInfo& GetCharInfo(wchar_t t)
		{
			auto it = infoByChar.find(t);
			if(it == infoByChar.end())
				return AddCharInfo(t);
			return infos[it->second];
		}
		Texture GetTextureMap()
		{
			if(bUpdated)
			{
				if(textureMap) // Dispose of old texture
					textureMap.Destroy();
				textureMap = spriteMap->GenerateTexture(m_gl);
				textureMap->SetFilter(true, true);
				bUpdated = false;
			}
			return textureMap;
		}
	private:
		const CharInfo& AddCharInfo(wchar_t t)
		{
			bUpdated = true;
			infoByChar.Add(t, (uint32)infos.size());
			infos.emplace_back();
			CharInfo& ci = infos.back();

			FT_Face* pFace = &face;

			ci.glyphID = FT_Get_Char_Index(*pFace, t);
			if(ci.glyphID == 0)
			{
				pFace = &fallbackFont;
				ci.glyphID = FT_Get_Char_Index(*pFace, t);
			}
			FT_Load_Glyph(*pFace, ci.glyphID, FT_LOAD_DEFAULT);

			if((*pFace)->glyph->format != FT_GLYPH_FORMAT_BITMAP)
			{
				FT_Render_Glyph((*pFace)->glyph, FT_RENDER_MODE_NORMAL);
			}

			ci.character = t;
			ci.offset.x = (*pFace)->glyph->bitmap_left;
			ci.offset.y = (*pFace)->glyph->bitmap_top;
			ci.advance = (float)(*pFace)->glyph->advance.x / 64.0f;

			Image img = ImageRes::Create(Vector2i((*pFace)->glyph->bitmap.width, (*pFace)->glyph->bitmap.rows));
			Colori* pDst = img->GetBits();
			uint8* pSrc = (*pFace)->glyph->bitmap.buffer;
			uint32 nLen = (*pFace)->glyph->bitmap.width * (*pFace)->glyph->bitmap.rows;
			for(uint32 i = 0; i < nLen; i++)
			{
				pDst[0].w = pSrc[0];
				Reinterpret<VectorBase<uint8, 3>>(pDst[0]) = VectorBase<uint8, 3>(255, 255, 255);
				pSrc++;
				pDst++;
			}
			uint32 nIndex = spriteMap->AddSegment(img);
			ci.coords = spriteMap->GetCoords(nIndex);

			return ci;
		}

		OpenGL* m_gl;
	};

	/*
		Vertex format used for text rendering
	*/
	struct TextVertex : public VertexFormat<Vector2, Vector2>
	{
		TextVertex(Vector2 point, Vector2 uv) : pos(point), tex(uv) {}
		Vector2 pos;
		Vector2 tex;
	};

	TextRes::~TextRes()
	{
	}
	Ref<class TextureRes> TextRes::GetTexture()
	{
		return fontSize->GetTextureMap();
	}
	Ref<class MeshRes> TextRes::GetMesh()
	{
		return mesh;
	}
	void TextRes::Draw()
	{
		GetTexture()->Bind();
		mesh->Draw();
	}

	class Font_Impl : public FontRes
	{
		FT_Face m_face;
		Buffer m_data;

		Map<uint32, FontSize*> m_sizes;
		uint32 m_currentSize = 0;

		OpenGL* m_gl;

		friend class TextRes;
	public:
		Font_Impl(class OpenGL* gl) : m_gl(gl)
		{
		}
		~Font_Impl()
		{
			for(auto s : m_sizes)
			{
				delete s.second;
			}
			m_sizes.clear();
			FT_Done_Face(m_face);
		}
		bool Init(const String& assetPath)
		{
			File in;
			if(!in.OpenRead(assetPath))
				return false;

			m_data.resize(in.GetSize());
			if(m_data.size() == 0)
				return false;

			in.Read(&m_data.front(), m_data.size());

			if(FT_New_Memory_Face(library, m_data.data(), (FT_Long)m_data.size(), 0, &m_face) != 0)
				return false;

			if(FT_Select_Charmap(m_face, FT_ENCODING_UNICODE) != 0)
				assert(false);

			return true;
		}

		FontSize* GetSize(uint32 size)
		{
			if(m_currentSize != size)
			{
				FT_Set_Pixel_Sizes(m_face, 0, size);
				m_currentSize = size;
			}
			if(fallbackFontSize != size)
			{
				FT_Set_Pixel_Sizes(fallbackFont, 0, size);
				fallbackFontSize = size;
			}

			auto it = m_sizes.find(size);
			if(it != m_sizes.end())
				return it->second;

			FontSize* fontSize = new FontSize(m_gl, m_face);
			fontSize->size = size;
			m_sizes.Add(size, fontSize);
			return fontSize;
		}
		
		inline GlyphInfo ProcessGlyph(FontSize* size, Vector<TextVertex>& vertices, wchar_t c, 
			Vector2& pen, Rect& boundary, Vector2& textSize, float monospaceWidth = 0.0f, bool useLineHeight = true)
		{
			GlyphInfo ret;
			bool monospaced = monospaceWidth > 0.0f;

			const CharInfo& info = size->GetCharInfo(c);

			if(info.coords.size.x != 0 && info.coords.size.y != 0)
			{
				Vector2 corners[4];
				corners[0] = Vector2(0, 0);
				corners[1] = Vector2((float)info.coords.size.x, 0);
				corners[2] = Vector2((float)info.coords.size.x, (float)info.coords.size.y);
				corners[3] = Vector2(0, (float)info.coords.size.y);

				Vector2 offset = Vector2(pen.x, pen.y);
				offset.x += info.offset.x;

				if(useLineHeight)
					offset.y += size->ascender - info.offset.y;
				else
					offset.y += info.offset.y;

				if(monospaced)
				{
					offset.x += (monospaceWidth - info.coords.size.x) * 0.5f;
				}
				pen.x = floorf(pen.x);
				pen.y = floorf(pen.y);

				vertices.emplace_back(offset + corners[2],
					corners[2] + info.coords.pos);
				vertices.emplace_back(offset + corners[0],
					corners[0] + info.coords.pos);
				vertices.emplace_back(offset + corners[1],
					corners[1] + info.coords.pos);

				vertices.emplace_back(offset + corners[3],
					corners[3] + info.coords.pos);
				vertices.emplace_back(offset + corners[0],
					corners[0] + info.coords.pos);
				vertices.emplace_back(offset + corners[2],
					corners[2] + info.coords.pos);

				boundary.Expand(offset + corners[0]);
				boundary.Expand(offset + corners[2]);
			}

			if(c == L'\n')
			{
				pen.x = 0.0f;
				pen.y += size->lineHeight;
				textSize.y = pen.y;
			}
			else if(c == L'\t')
			{
				const CharInfo& space = size->GetCharInfo(L' ');
				pen.x += space.advance * 3.0f;
			}
			else
			{
				if(monospaced)
				{
					pen.x += monospaceWidth;
				}
				else
					pen.x += info.advance;
			}
			textSize.x = std::max(textSize.x, pen.x);

			return ret;
		}

		Text CreateTextInternal(const WString& str, uint32 nFontSize, float monospaceWidth)
		{
			FontSize* size = GetSize(nFontSize);

			Text cachedText = size->cache.GetText(str);
			if(cachedText)
				return cachedText;

			TextRes* ret = new TextRes();
			ret->mesh = MeshRes::Create(m_gl);
			ret->textBounds = Rect::Empty;

			Vector<TextVertex> vertices;
			Vector2 pen;
			for(wchar_t c : str)
			{
				ProcessGlyph(size, vertices, c, pen, ret->textBounds, ret->size, monospaceWidth, true);
			}

			ret->size.y += size->lineHeight;

			ret->fontSize = size;
			ret->mesh->SetData(vertices);
			ret->mesh->SetPrimitiveType(PrimitiveType::TriangleList);

			Text textObj = Ref<TextRes>(ret);
			// Insert into cache
			size->cache.AddText(str, textObj);
			return textObj;
		}

		Text CreateText(const WString& str, uint32 fontSize)
		{
			return CreateTextInternal(str, fontSize, 0.0f);
		}
		Text CreateTextMonospaced(const WString& str, uint32 fontSize, float monospaceWidth)
		{
			return CreateTextInternal(str, fontSize, monospaceWidth);
		}
		Text CreateSingle(wchar_t c, uint32 fontSize, GlyphInfo& info)
		{
			FontSize* size = GetSize(fontSize);
			//sinfo = ProcessGlyph()
			//s
			//sVector<TextVertex> vertices;
			//sVector2 pen;
			//sfor(wchar_t c : str)
			//s{
			//s	ProcessGlyph(size, vertices, c, pen, ret->size, monospaceWidth);
			//s}
			return Text();
		}
	};

	Font FontRes::Create(OpenGL* gl, const String& assetPath)
	{
		Font_Impl* pImpl = new Font_Impl(gl);
		if(pImpl->Init(assetPath))
		{
			return GetResourceManager<ResourceType::Font>().Register(pImpl);
		}
		else
		{
			delete pImpl;
			return Font();
		}
	}

	bool FontLibrary::LoadFallbackFont()
	{
		bool success = true;

		// Load fallback font
		File file;
		if(!file.OpenRead("fonts/fallbackfont.otf"))
			return false;
		loadedFallbackFont.resize(file.GetSize());
		file.Read(loadedFallbackFont.data(), loadedFallbackFont.size());
		file.Close();

		success = success && FT_New_Memory_Face(library, loadedFallbackFont.data(), (uint32)loadedFallbackFont.size(), 0, &fallbackFont) == 0;
		success = success && FT_Select_Charmap(fallbackFont, FT_ENCODING_UNICODE) == 0;
		return success;
	}
	FontLibrary::FontLibrary()
	{
		bool success = true;
		success = success && FT_Init_FreeType(&library) == FT_Err_Ok;
		if(!LoadFallbackFont())
		{
			Log("Failed to load embeded fallback font", Logger::Warning);
		}
		if(!success)
			assert(false);
	}
	FontLibrary::~FontLibrary()
	{
		FT_Done_Face(fallbackFont);
		FT_Done_FreeType(library);
	}
}