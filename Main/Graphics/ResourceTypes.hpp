#pragma once

// Macro used to create specializations for the ResourceManagerTypes struct
#define DEFINE_RESOURCE_TYPE(_EnumMember, _Type)\
template<> struct ResourceManagerTypes<ResourceType::_EnumMember>\
{\
	typedef _Type Type;\
};

enum class ResourceType
{
	Image = 0,
	SpriteMap,
	Texture,
	Framebuffer,
	Font,
	Mesh,
	Shader,
	Material,
	_Length
};

template<ResourceType E>
struct ResourceManagerTypes
{
};