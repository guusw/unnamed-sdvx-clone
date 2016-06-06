#pragma once
#include <Graphics/Image.hpp>

/*
	Windows bitmap abstraction
*/
class HBitmap : public Unique
{
	Image m_source;
	HBITMAP m_handle;
	HDC m_dc;
	Vector2i m_size;
	void* m_data;
public:
	HBitmap(const Vector2i &size, HDC dc) : m_dc(dc)
	{
		m_CreateBitmap(size);
	}
	HBitmap(Image srcImg, HDC dc) : m_dc(dc)
	{
		assert(srcImg.IsValid());
		m_source = srcImg;
		m_CreateBitmap(m_source->GetSize());

		uint32 dataLen = sizeof(Colori) * m_size.x * m_size.y;
		memcpy(m_data, m_source->GetBits(), dataLen);
	}
	~HBitmap()
	{
		DeleteObject(m_handle);
	}
	const Vector2i& GetSize() const
	{
		return m_size;
	}
	operator HGDIOBJ()
	{
		return (HGDIOBJ)m_handle;
	}
	operator HBITMAP()
	{
		return m_handle;
	}

private:
	void m_CreateBitmap(const Vector2i& size)
	{
		BITMAPINFO dib = { 0 };
		dib.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
		dib.bmiHeader.biWidth = size.x;
		dib.bmiHeader.biHeight = size.y;
		dib.bmiHeader.biPlanes = 1;
		dib.bmiHeader.biBitCount = 32;
		dib.bmiHeader.biCompression = BI_RGB;

		m_size = size;
		HBITMAP hbm = m_handle = CreateDIBSection(m_dc, &dib, DIB_RGB_COLORS, &m_data, NULL, 0);
		assert(hbm);
	}
};