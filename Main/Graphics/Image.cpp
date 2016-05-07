#include "stdafx.h"
#include "Image.hpp"
#include "ResourceManager.hpp"
#include "ImageLoader.hpp"

class Image_Impl : public ImageRes
{
	Vector2i m_size;
	Colori* m_pData = nullptr;
	size_t m_nDataLength;
public:
	Image_Impl()
	{

	}
	~Image_Impl()
	{
		Clear();
	}
	void Clear()
	{
		if (m_pData)
			delete[] m_pData;
		m_pData = nullptr;
	}
	void Allocate()
	{
		m_nDataLength = m_size.x * m_size.y;
		if (m_nDataLength == 0)
			return;
		m_pData = new Colori[m_nDataLength];
	}

	void SetSize(Vector2i size)
	{
		m_size = size;
		Clear();
		Allocate();
	}
	Vector2i GetSize() const
	{
		return m_size;
	}
	Colori* GetBits()
	{
		return m_pData;
	}
	const Colori* GetBits() const
	{
		return m_pData;
	}
};

Image ImageRes::Create(Vector2i size)
{
	Image_Impl* pImpl = new Image_Impl();
	pImpl->SetSize(size);
	return GetResourceManager<ResourceType::Image>().Register(pImpl);
}
Image ImageRes::Create(const String& assetPath)
{
	Image_Impl* pImpl = new Image_Impl();
	if (ImageLoader::Load(pImpl, assetPath))
	{
		return GetResourceManager<ResourceType::Image>().Register(pImpl);
	}
	else 
	{
		delete pImpl;
		pImpl = nullptr;
	}
	return Image();
}