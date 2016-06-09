#include "stdafx.h"
#include "OpenGL.hpp"
#include "Texture.hpp"
#include "Image.hpp"
#include <Graphics/ResourceManagers.hpp>

namespace Graphics
{
	class Texture_Impl : public TextureRes
	{
		uint32 m_texture = 0;
		TextureFormat m_format = TextureFormat::Invalid;
		Vector2i m_size;
		bool m_filter = true;
		bool m_mipFilter = true;
		bool m_mipmaps = false;
		float m_anisotropic = 1.0f;
	public:
		Texture_Impl()
		{
		}
		void Dispose()
		{
			if(m_texture)
				glDeleteTextures(1, &m_texture);
		}
		bool Init()
		{
			glGenTextures(1, &m_texture);
			if(m_texture == 0)
				return false;
			return true;
		}
		virtual void Init(Vector2i size, TextureFormat format)
		{
			m_format = format;
			m_size = size;

			uint32 ifmt = -1;
			uint32 fmt = -1;
			uint32 type = -1;
			if(format == TextureFormat::D32)
			{
				ifmt = GL_DEPTH_COMPONENT32;
				fmt = GL_DEPTH_COMPONENT;
				type = GL_FLOAT;
			}
			else if(format == TextureFormat::RGBA8)
			{
				ifmt = GL_RGBA8;
				fmt = GL_RGBA;
				type = GL_UNSIGNED_BYTE;
			}
			else
			{
				assert(false);
			}

			glTextureImage2DEXT(m_texture, GL_TEXTURE_2D, 0, ifmt, size.x, size.y, 0, fmt, type, nullptr);
			UpdateFilterState();
		}
		virtual void SetData(Vector2i size, void* pData)
		{
			m_format = TextureFormat::RGBA8;
			m_size = size;
			glTextureImage2DEXT(m_texture, GL_TEXTURE_2D, 0, GL_RGBA8, size.x, size.y, 0, GL_RGBA, GL_UNSIGNED_BYTE, pData);
			UpdateFilterState();
		}
		void UpdateFilterState()
		{
			if(!m_mipmaps)
			{
				glTextureParameteri(m_texture, GL_TEXTURE_MIN_FILTER, m_filter ? GL_LINEAR : GL_NEAREST);
				glTextureParameteri(m_texture, GL_TEXTURE_MAG_FILTER, m_filter ? GL_LINEAR : GL_NEAREST);
			}
			else
			{
				if(m_mipFilter)
					glTextureParameteri(m_texture, GL_TEXTURE_MIN_FILTER, m_filter ? GL_LINEAR_MIPMAP_LINEAR : GL_NEAREST_MIPMAP_LINEAR);
				else
					glTextureParameteri(m_texture, GL_TEXTURE_MIN_FILTER, m_filter ? GL_LINEAR_MIPMAP_NEAREST : GL_NEAREST_MIPMAP_NEAREST);
				glTextureParameteri(m_texture, GL_TEXTURE_MAG_FILTER, m_filter ? GL_LINEAR : GL_NEAREST);
			}

			if(GL_TEXTURE_MAX_ANISOTROPY_EXT)
			{
				glTextureParameterf(m_texture, GL_TEXTURE_MAX_ANISOTROPY_EXT, m_anisotropic);
			}
		}
		virtual void SetFilter(bool enabled, bool mipFiltering, float anisotropic)
		{
			m_mipFilter = mipFiltering;
			m_filter = enabled;
			m_anisotropic = anisotropic;
			assert(m_anisotropic >= 1.0f && m_anisotropic <= 16.0f);
			UpdateFilterState();
		}
		virtual void SetMipmaps(bool enabled)
		{
			if(enabled)
			{
				glGenerateTextureMipmap(m_texture);
			}
			m_mipmaps = enabled;
			UpdateFilterState();
		}
		virtual const Vector2i& GetSize() const
		{
			return m_size;
		}
		virtual void Bind(uint32 index)
		{
			glActiveTexture(GL_TEXTURE0 + index);
			glBindTexture(GL_TEXTURE_2D, m_texture);
		}
		virtual uint32 Handle()
		{
			return m_texture;
		}

		virtual void SetWrap(TextureWrap u, TextureWrap v) override
		{
			uint32 wmode[] = {
				GL_REPEAT,
				GL_MIRRORED_REPEAT,
				GL_CLAMP_TO_EDGE,
			};
			glTextureParameteri(m_texture, GL_TEXTURE_WRAP_S, wmode[(size_t)u]);
			glTextureParameteri(m_texture, GL_TEXTURE_WRAP_T, wmode[(size_t)v]);
		}

		TextureFormat GetFormat() const
		{
			return m_format;
		}
	};

	Texture TextureRes::Create(OpenGL* gl)
	{
		Texture_Impl* pImpl = new Texture_Impl();
		if(pImpl->Init())
		{
			return GetResourceManager<ResourceType::Texture>().Register(pImpl);
		}
		else
		{
			delete pImpl; pImpl = nullptr;
		}
		return Texture();
	}
	Texture TextureRes::Create(OpenGL* gl, Image image)
	{
		if(!image)
			return Texture();
		Texture_Impl* pImpl = new Texture_Impl();
		if(pImpl->Init())
		{
			pImpl->SetData(image->GetSize(), image->GetBits());
			return GetResourceManager<ResourceType::Texture>().Register(pImpl);
		}
		else
		{
			delete pImpl;
			return Texture();
		}
	}

	float TextureRes::CalculateHeight(float width)
	{
		Vector2 size = GetSize();
		float aspect = size.y / size.x;
		return aspect * width;
	}

	float TextureRes::CalculateWidth(float height)
	{
		Vector2 size = GetSize();
		float aspect = size.x / size.y;
		return aspect * height;
	}
}
