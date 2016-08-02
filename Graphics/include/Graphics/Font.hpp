#pragma once
#include <Graphics/ResourceTypes.hpp>

// This is the resource id for the default font on windows embeded into the application resource
#define IDR_FALLBACKFONT                    101

namespace Graphics
{
	/*
		A prerendered text object, contains all the vertices and texture sheets to draw itself
	*/
	class TextRes
	{
		friend class Font_Impl;
		Ref<class TextureRes> spriteMap;
		Ref<class MeshRes> mesh;
	public:
		~TextRes();
		Ref<class TextureRes> GetTexture() { return spriteMap; }
		Ref<class MeshRes> GetMesh() { return mesh; }
		void Draw();
		Vector2 size;
	};

	/*
		Font class, can create Text objects
	*/
	class FontRes
	{
	public:
		virtual ~FontRes() = default;
		static Ref<FontRes> Create(class OpenGL* gl, const String& assetPath);
	public:
		virtual Ref<TextRes> CreateText(const WString& str, uint32 nFontSize) = 0;
	};

    typedef Ref<FontRes> GlFont;
	typedef Ref<TextRes> Text;

    DEFINE_RESOURCE_TYPE(GlFont, FontRes);
}