#pragma once
#include <Graphics/Image.hpp>

namespace Graphics
{
	/*
		Window style customization.
		this allows for things like custom window borders.
	*/
	class CustomWindowStyle
	{
	public:
		CustomWindowStyle();

		bool enabled = false;

		// The image used for the border
		Image borderImage;

		// The size of the border around the window client area
		Margini borderMargin;

		static CustomWindowStyle Default;
	};
}