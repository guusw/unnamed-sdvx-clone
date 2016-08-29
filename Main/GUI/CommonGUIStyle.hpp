#pragma once

/*
	Style for common GUI elements such as buttons,sliders and text fields
*/
class CommonGUIStyle
{
public:
	CommonGUIStyle() = default;
	CommonGUIStyle(class Application* application);
	static Ref<CommonGUIStyle> instance;
	static Ref<CommonGUIStyle> Get();

	Margini buttonPadding = Margini(5);
	Margini buttonBorder = Margini(5);
	Texture buttonTexture;
	Texture buttonHighlightTexture;

	Margini sliderBorder = Margini(0);
	Margini sliderButtonPadding;
	Texture sliderTexture;
	Texture sliderButtonTexture;
	Texture sliderButtonHighlightTexture;

	Margini verticalSliderBorder = Margini(0);
	Margini verticalSliderButtonPadding;
	Texture verticalSliderTexture;
	Texture verticalSliderButtonTexture;
	Texture verticalSliderButtonHighlightTexture;

	Margini textfieldPadding = Margini(5);
	Margini textfieldBorder = Margini(5);
	Texture textfieldTexture;
	Texture textfieldHighlightTexture;

	Texture spinnerTexture;
};