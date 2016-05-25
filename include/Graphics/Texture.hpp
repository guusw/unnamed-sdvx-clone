#pragma once
#include <Graphics/ResourceTypes.hpp>

enum class TextureWrap
{
	Repeat,
	Mirror,
	Clamp,
};

enum class TextureFormat
{
	RGBA8,
	D32,
	Invalid
};

class ImageRes;
class TextureRes
{
public:
	virtual ~TextureRes() = default;
	static Ref<TextureRes> Create(class OpenGL* gl);
	static Ref<TextureRes> Create(class OpenGL* gl, Ref<class ImageRes> image);
public:
	virtual void Init(Vector2i size, TextureFormat format = TextureFormat::RGBA8) = 0;
	virtual void SetData(Vector2i size, void* pData) = 0;
	virtual void SetMipmaps(bool enabled) = 0;
	virtual void SetFilter(bool enabled, bool mipFiltering = true, float anisotropic = 1.0f) = 0;
	virtual const Vector2i& GetSize() const = 0;

	// Gives the aspect ratio correct height for a given width
	float CalculateHeight(float width);
	// Gives the aspect ratio correct width for a given height
	float CalculateWidth(float height);

	virtual void Bind(uint32 index = 0) = 0;
	virtual uint32 Handle() = 0;
	virtual void SetWrap(TextureWrap u, TextureWrap v) = 0;
	virtual TextureFormat GetFormat() const = 0;
};

typedef Ref<TextureRes> Texture;

DEFINE_RESOURCE_TYPE(Texture, TextureRes);