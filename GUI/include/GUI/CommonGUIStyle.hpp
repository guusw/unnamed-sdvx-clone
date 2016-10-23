#pragma once

namespace Graphics
{
	class OpenGL;
}

/*
	Style for common GUI elements such as buttons,sliders and text fields
*/
class CommonGUIStyle
{
public:
	CommonGUIStyle() = default;
	CommonGUIStyle(Graphics::OpenGL* gl);

	Margini buttonPadding = Margini(5);
	Margini buttonBorder = Margini(5);
	Graphics::Texture buttonTexture;
	Graphics::Texture buttonHighlightTexture;

	Margini sliderBorder = Margini(0);
	Margini sliderButtonPadding;
	Graphics::Texture sliderTexture;
	Graphics::Texture sliderButtonTexture;
	Graphics::Texture sliderButtonHighlightTexture;

	Margini verticalSliderBorder = Margini(0);
	Margini verticalSliderButtonPadding;
	Graphics::Texture verticalSliderTexture;
	Graphics::Texture verticalSliderButtonTexture;
	Graphics::Texture verticalSliderButtonHighlightTexture;

	Margini textfieldPadding = Margini(5);
	Margini textfieldBorder = Margini(5);
	Graphics::Texture textfieldTexture;
	Graphics::Texture textfieldHighlightTexture;

	Graphics::Texture spinnerTexture;
};