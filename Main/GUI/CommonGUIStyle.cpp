#include "stdafx.h"
#include "CommonGUIStyle.hpp"
#include "Application.hpp"

CommonGUIStyle::CommonGUIStyle(Application* application)
{
	buttonTexture = application->LoadTexture("ui/button.png");
	buttonHighlightTexture = application->LoadTexture("ui/button_hl.png");
	buttonPadding = Margini(10);
	buttonBorder = Margini(5);

	sliderButtonTexture = application->LoadTexture("ui/slider_button.png");
	sliderButtonHighlightTexture = application->LoadTexture("ui/slider_button_hl.png");
	sliderTexture = application->LoadTexture("ui/slider.png");
	sliderBorder = Margini(8, 0);
	sliderButtonPadding = Margini(-15, -8);

	verticalSliderButtonTexture = application->LoadTexture("ui/vslider_button.png");
	verticalSliderButtonHighlightTexture = application->LoadTexture("ui/vslider_button_hl.png");
	verticalSliderTexture = application->LoadTexture("ui/vslider.png");
	verticalSliderBorder = Margini(0, 8);
	verticalSliderButtonPadding = Margini(-8, -15);

	textfieldTexture = application->LoadTexture("ui/textfield.png");
	textfieldHighlightTexture = application->LoadTexture("ui/textfield_hl.png");
	textfieldPadding = Margini(10);
	textfieldBorder = Margini(5);

	spinnerTexture = application->LoadTexture("ui/spinner.png");
	spinnerTexture->SetWrap(TextureWrap::Clamp, TextureWrap::Clamp);
}
