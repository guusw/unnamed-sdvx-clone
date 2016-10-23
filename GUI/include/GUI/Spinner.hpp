#pragma once
#include <GUI/GUIElement.hpp>
#include <GUI/CommonGUIStyle.hpp>

/*
	Spinning image used for loading images
*/
class Spinner : public GUIElementBase
{
public:
	Spinner(Ref<CommonGUIStyle> style);
	virtual void Render(GUIRenderData data) override;
	virtual Vector2 m_GetDesiredBaseSize(GUIRenderData rd) override;

private:
	float m_rotation = 0.0f;
	Ref<CommonGUIStyle> m_style;
};