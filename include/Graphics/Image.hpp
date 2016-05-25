#pragma once
#include <Graphics/ResourceTypes.hpp>

class ImageRes
{
public:
	virtual ~ImageRes() = default;
	static Ref<ImageRes> Create(const String& assetPath);
	static Ref<ImageRes> Create(Vector2i size = Vector2i());
public:
	virtual void SetSize(Vector2i size) = 0;
	virtual Vector2i GetSize() const = 0;
	virtual Colori* GetBits() = 0;
	virtual const Colori* GetBits() const = 0;
};

class TextureRes;
class SpriteMapRes : public ImageRes
{
public:
	virtual ~SpriteMapRes() = default;
	static Ref<SpriteMapRes> Create();
public:
	virtual uint32 AddSegment(Ref<ImageRes> image) = 0;
	virtual void Clear() = 0;
	virtual Ref<ImageRes> Generate() = 0;
	virtual Ref<class TextureRes> GenerateTexture(class OpenGL* gl) = 0;
	virtual Recti GetCoords(uint32 nIndex) = 0;
};

typedef Ref<ImageRes> Image;
typedef Ref<SpriteMapRes> SpriteMap;

DEFINE_RESOURCE_TYPE(Image, ImageRes);
DEFINE_RESOURCE_TYPE(SpriteMap, SpriteMapRes);