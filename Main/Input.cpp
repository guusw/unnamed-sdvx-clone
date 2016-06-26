#include "stdafx.h"
#include "Input.hpp"

void Input::Init(Window& wnd)
{
	Cleanup();
	m_window = &wnd;
	m_window->OnKeyPressed.Add(this, &Input::OnKeyPressed);
	m_window->OnKeyReleased.Add(this, &Input::OnKeyReleased);

	m_InitButtonMapping();
}
void Input::Cleanup()
{
	if(m_window)
	{
		m_window->OnKeyPressed.RemoveAll(this);
		m_window->OnKeyReleased.RemoveAll(this);
		m_window = nullptr;
	}
}

bool Input::GetButton(Button button) const
{
	return m_buttonStates[(size_t)button];
}

float Input::GetInputLaserDir(uint32 laserIdx)
{
	size_t base = (size_t)Button::LS_0Neg;
	if(laserIdx == 1)
	{
		base = (size_t)Button::LS_1Neg;
	}
	float r = 0.0f;
	r -= 1.0f * m_buttonStates[base];
	r += 1.0f * m_buttonStates[base + 1];
	return r;
}
void Input::m_InitButtonMapping()
{
	memset(m_buttonStates, 0, sizeof(m_buttonStates));
	m_buttonMap.clear();

	// Default KShoot mapping
	m_buttonMap[(Key)'S'] = Button::BT_0;
	m_buttonMap[(Key)'D'] = Button::BT_1;
	m_buttonMap[(Key)'K'] = Button::BT_2;
	m_buttonMap[(Key)'L'] = Button::BT_3;
	// Alternate button mappings
	m_buttonMap[(Key)'H'] = Button::BT_0;
	m_buttonMap[(Key)'J'] = Button::BT_1;
	m_buttonMap[(Key)'F'] = Button::BT_2;
	m_buttonMap[(Key)'G'] = Button::BT_3;

	m_buttonMap[(Key)'C'] = Button::FX_0;
	m_buttonMap[(Key)'M'] = Button::FX_1;
	// Alternate button mappings
	m_buttonMap[(Key)'N'] = Button::FX_0;
	m_buttonMap[(Key)'V'] = Button::FX_1;

	m_buttonMap[(Key)'W'] = Button::LS_0Neg;
	m_buttonMap[(Key)'E'] = Button::LS_0Pos;
	m_buttonMap[(Key)'O'] = Button::LS_1Neg;
	m_buttonMap[(Key)'P'] = Button::LS_1Pos;
}
void Input::m_OnButtonInput(Button b, bool pressed)
{
	bool& state = m_buttonStates[(size_t)b];
	if(state != pressed)
	{
		state = pressed;
		if(state)
		{
			OnButtonPressed.Call(b);
		}
		else
		{
			OnButtonReleased.Call(b);
		}
	}
}
void Input::OnKeyPressed(Key key)
{
	// Handle button mappings
	auto it = m_buttonMap.find(key);
	if(it != m_buttonMap.end())
		m_OnButtonInput(it->second, true);
}
void Input::OnKeyReleased(Key key)
{
	// Handle button mappings
	auto it = m_buttonMap.find(key);
	if(it != m_buttonMap.end())
		m_OnButtonInput(it->second, false);
}