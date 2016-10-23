#include "stdafx.h"
#include "CommonGUIStyle.hpp"
using namespace Graphics;

CommonGUIStyle::CommonGUIStyle(class OpenGL* gl)
{
	auto LoadTexture = [&](const String& path)
	{
		String fullPath = Path::Normalize(String("textures/ui/") + path);
		Image img = ImageRes::Create(fullPath);
		if(!img)
			return Texture();
		return TextureRes::Create(gl, img);
	};

	buttonTexture = LoadTexture("button.png");
	buttonHighlightTexture = LoadTexture("button_hl.png");
	buttonPadding = Margini(10);
	buttonBorder = Margini(5);

	sliderButtonTexture = LoadTexture("slider_button.png");
	sliderButtonHighlightTexture = LoadTexture("slider_button_hl.png");
	sliderTexture = LoadTexture("slider.png");
	sliderBorder = Margini(8, 0);
	sliderButtonPadding = Margini(-15, -8);

	verticalSliderButtonTexture = LoadTexture("vslider_button.png");
	verticalSliderButtonHighlightTexture = LoadTexture("vslider_button_hl.png");
	verticalSliderTexture = LoadTexture("vslider.png");
	verticalSliderBorder = Margini(0, 8);
	verticalSliderButtonPadding = Margini(-8, -15);

	textfieldTexture = LoadTexture("textfield.png");
	textfieldHighlightTexture = LoadTexture("textfield_hl.png");
	textfieldPadding = Margini(10);
	textfieldBorder = Margini(5);

	spinnerTexture = LoadTexture("spinner.png");
	spinnerTexture->SetWrap(TextureWrap::Clamp, TextureWrap::Clamp);
}
