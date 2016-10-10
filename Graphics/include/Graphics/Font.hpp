#pragma once
#include <Graphics/ResourceTypes.hpp>

#ifdef None
#undef None
#endif

namespace Graphics
{
	// Individual glyph info
	class GlyphInfo
	{
	public:
		float advance;
		Vector2i offset;
		Recti coords;
		wchar_t character;
	};

	/*
		A prerendered text object, contains all the vertices and texture sheets to draw itself
	*/
	class TextRes
	{
		friend class Font_Impl;
		struct FontSize* fontSize;
		Ref<class MeshRes> mesh;
	public:
		~TextRes();
		Ref<class TextureRes> GetTexture();
		Ref<class MeshRes> GetMesh();
		void Draw();


		Vector2 size;
		Rect textBounds;
		struct CharacterInfo
		{
			Vector2 position;
			GlyphInfo glyph;
		};
		Vector<CharacterInfo> characters;
	};
	typedef Ref<TextRes> Text;

	/*
		Font class, can create Text objects
	*/
	class FontRes
	{
	public:
		virtual ~FontRes() = default;
		static Ref<FontRes> Create(class OpenGL* gl, const String& assetPath);
	public:
		// Renders the input string into a drawable text object
		virtual Text CreateText(const WString& str, uint32 fontSize) = 0;

		// Render input string into monospaced text
		virtual Text CreateTextMonospaced(const WString& str, uint32 fontSize, float monospaceWidth) = 0;

		// Creats a single glyph
		virtual Text CreateSingle(wchar_t c, uint32 fontSize, GlyphInfo& info) = 0;
	};

	typedef Ref<FontRes> Font;

	DEFINE_RESOURCE_TYPE(Font, FontRes);
}
