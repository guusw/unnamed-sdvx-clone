#include "stdafx.h"
#include "Input.hpp"
#include "GameConfig.hpp"
#include "SDL_joystick.h"
#include "SDL_gamecontroller.h"

/* 
	Wrapper around SDL_GameController 
*/
class Controller_Impl : Unique
{
private:
	SDL_GameController* m_controller;
public:
	Controller_Impl(SDL_GameController* controller) : m_controller(controller)
	{
	}
	~Controller_Impl()
	{
		if(m_controller)
			SDL_GameControllerClose(m_controller);
	}
	bool GetButton(uint32 idx)
	{
		uint8 state = SDL_GameControllerGetButton(m_controller, (SDL_GameControllerButton)idx);
		return state != 0;
	}
	float GetAxis(uint32 idx)
	{
		float val = (float)SDL_GameControllerGetAxis(m_controller, (SDL_GameControllerAxis)idx) / 0x7fff;
		return val;
	}
};

Input::~Input()
{
	// Shoud be set to null by Cleanup
	assert(!m_controller);
	assert(!m_window);
}

void Input::Init(Graphics::Window& wnd)
{
	Cleanup();
	m_window = &wnd;
	m_window->OnKeyPressed.Add(this, &Input::OnKeyPressed);
	m_window->OnKeyReleased.Add(this, &Input::OnKeyReleased);

	m_lastMousePos = m_window->GetMousePos();

	m_laserDevice = g_gameConfig.GetEnum<Enum_InputDevice>(GameConfigKeys::LaserInputDevice);
	m_buttonDevice = g_gameConfig.GetEnum<Enum_InputDevice>(GameConfigKeys::ButtonInputDevice);

	m_mouseAxisMapping[0] = g_gameConfig.Get<int32>(GameConfigKeys::Mouse_Laser0Axis);
	m_mouseAxisMapping[1] = g_gameConfig.Get<int32>(GameConfigKeys::Mouse_Laser1Axis);
	m_mouseSensitivity = g_gameConfig.Get<float>(GameConfigKeys::Mouse_Sensitivity);

	m_controllerAxisMapping[0] = g_gameConfig.Get<int32>(GameConfigKeys::Controller_Laser0Axis);
	m_controllerAxisMapping[1] = g_gameConfig.Get<int32>(GameConfigKeys::Controller_Laser1Axis);
	m_controllerSensitivity = g_gameConfig.Get<float>(GameConfigKeys::Controller_Sensitivity);
	m_controllerDeadzone = g_gameConfig.Get<float>(GameConfigKeys::Controller_Deadzone);

	// Init controller mapping
	if(m_laserDevice == InputDevice::Controller || m_buttonDevice == InputDevice::Controller)
	{
		int32 numControllers = SDL_NumJoysticks();

		int32 deviceIndex = g_gameConfig.Get<int32>(GameConfigKeys::Controller_DeviceID);
		if(deviceIndex >= numControllers)
		{
			Logf("Out of range controller [%d], number of available controllers is %d", Logger::Error, deviceIndex, numControllers);
		}
		else
		{
			SDL_GameController* controller = SDL_GameControllerOpen(deviceIndex);
			m_controller = new Controller_Impl(controller);
		}
		m_InitControllerMapping();
	}

	// Init keyboard mapping
	m_InitKeyboardMapping();
}
void Input::Cleanup()
{
	if(m_controller)
	{
		delete m_controller;
		m_controller = nullptr;
	}
	if(m_window)
	{
		m_window->OnKeyPressed.RemoveAll(this);
		m_window->OnKeyReleased.RemoveAll(this);
		m_window = nullptr;
	}
}

void Input::Update(float deltaTime)
{
	for(auto it = m_mouseLocks.begin(); it != m_mouseLocks.end();)
	{
		if(it->GetRefCount() == 1)
		{
			it = m_mouseLocks.erase(it);
			continue;
		}
		it++;
	}

	Vector2i mousePos = m_window->GetMousePos();
	Vector2 mouseDelta = mousePos - m_lastMousePos;
	m_lastMousePos = mousePos;
	float* mouseAxes = &mouseDelta.x;

	// Reset mouse to edge of screen
	if(!m_mouseLocks.empty())
	{
		Vector2 mouseCenter = m_window->GetWindowSize() / 2;
		m_window->SetMousePos(mouseCenter);
		m_lastMousePos = mouseCenter;
	}
	
	if(m_laserDevice == InputDevice::Mouse)
	{
		for(uint32 i = 0; i < 2; i++)
		{
			if(m_mouseAxisMapping[i] < 0 || m_mouseAxisMapping[i] > 1)
			{
				// INVALID MAPPING
				m_laserStates[i] = 0.0f;
				continue;
			}
			
			m_laserStates[i] = mouseAxes[m_mouseAxisMapping[i]] * m_mouseSensitivity;
		}
	}


	if(m_controller)
	{
		// Poll controller button input
		for(auto m : m_controllerMap)
		{
			bool newState = m_controller->GetButton(m.first);
			m_OnButtonInput(m.second, newState);
		}

		// Poll controller laser input
		if(m_laserDevice == InputDevice::Controller)
		{
			for(uint32 i = 0; i < 2; i++)
			{
				m_laserStates[i] = m_controller->GetAxis(m_controllerAxisMapping[i]) * m_controllerSensitivity;
				if(abs(m_laserStates[i]) < m_controllerDeadzone)
					m_laserStates[i] = 0.0f;
			}
		}
	}
}

bool Input::GetButton(Button button) const
{
	return m_buttonStates[(size_t)button];
}

Ref<int32> Input::LockMouse()
{
	return m_mouseLocks.Add(MouseLockHandle(new int32(m_mouseLockIndex++)));
}

float Input::GetInputLaserDir(uint32 laserIdx)
{
	return m_laserStates[laserIdx];
}
void Input::m_InitKeyboardMapping()
{
	memset(m_buttonStates, 0, sizeof(m_buttonStates));
	m_buttonMap.clear();

	if(m_buttonDevice == InputDevice::Keyboard)
	{
		// Button mappings
		m_buttonMap.Add(g_gameConfig.GetEnum<Enum_Key>(GameConfigKeys::Key_BT0), Button::BT_0);
		m_buttonMap.Add(g_gameConfig.GetEnum<Enum_Key>(GameConfigKeys::Key_BT1), Button::BT_1);
		m_buttonMap.Add(g_gameConfig.GetEnum<Enum_Key>(GameConfigKeys::Key_BT2), Button::BT_2);
		m_buttonMap.Add(g_gameConfig.GetEnum<Enum_Key>(GameConfigKeys::Key_BT3), Button::BT_3);
		// Alternate button mappings
		m_buttonMap.Add(g_gameConfig.GetEnum<Enum_Key>(GameConfigKeys::Key_BT0Alt), Button::BT_0);
		m_buttonMap.Add(g_gameConfig.GetEnum<Enum_Key>(GameConfigKeys::Key_BT1Alt), Button::BT_1);
		m_buttonMap.Add(g_gameConfig.GetEnum<Enum_Key>(GameConfigKeys::Key_BT2Alt), Button::BT_2);
		m_buttonMap.Add(g_gameConfig.GetEnum<Enum_Key>(GameConfigKeys::Key_BT3Alt), Button::BT_3);

		m_buttonMap.Add(g_gameConfig.GetEnum<Enum_Key>(GameConfigKeys::Key_FX0), Button::FX_0);
		m_buttonMap.Add(g_gameConfig.GetEnum<Enum_Key>(GameConfigKeys::Key_FX1), Button::FX_1);
		// Alternate button mappings
		m_buttonMap.Add(g_gameConfig.GetEnum<Enum_Key>(GameConfigKeys::Key_FX0Alt), Button::FX_0);
		m_buttonMap.Add(g_gameConfig.GetEnum<Enum_Key>(GameConfigKeys::Key_FX1Alt), Button::FX_1);
	}

	if(m_laserDevice == InputDevice::Keyboard)
	{
		// Laser button mappings
		m_buttonMap.Add(g_gameConfig.GetEnum<Enum_Key>(GameConfigKeys::Key_Laser0Neg), Button::LS_0Neg);
		m_buttonMap.Add(g_gameConfig.GetEnum<Enum_Key>(GameConfigKeys::Key_Laser0Pos), Button::LS_0Pos);
		m_buttonMap.Add(g_gameConfig.GetEnum<Enum_Key>(GameConfigKeys::Key_Laser1Neg), Button::LS_1Neg);
		m_buttonMap.Add(g_gameConfig.GetEnum<Enum_Key>(GameConfigKeys::Key_Laser1Pos), Button::LS_1Pos);
	}
}

void Input::m_InitControllerMapping()
{
	if(m_buttonDevice == InputDevice::Controller)
	{
		m_controllerMap.Add(g_gameConfig.Get<int32>(GameConfigKeys::Controller_BT0), Button::BT_0);
		m_controllerMap.Add(g_gameConfig.Get<int32>(GameConfigKeys::Controller_BT1), Button::BT_1);
		m_controllerMap.Add(g_gameConfig.Get<int32>(GameConfigKeys::Controller_BT2), Button::BT_2);
		m_controllerMap.Add(g_gameConfig.Get<int32>(GameConfigKeys::Controller_BT3), Button::BT_3);
		m_controllerMap.Add(g_gameConfig.Get<int32>(GameConfigKeys::Controller_FX0), Button::FX_0);
		m_controllerMap.Add(g_gameConfig.Get<int32>(GameConfigKeys::Controller_FX1), Button::FX_1);
	}
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

	static Timer t;
	if(b >= Button::LS_0Neg)
	{
		int32 btnIdx = (int32)b - (int32)Button::LS_0Neg;
		int32 laserIdx = btnIdx / 2;
		// Set laser state based uppon the button that was pressed last
		if(pressed)
			m_laserStates[laserIdx] = (btnIdx % 2) == 0 ? -1.0f : 1.0f;
		else // If a button was released check if the other one is still held
		{
			if(m_buttonStates[(int32)Button::LS_0Neg + laserIdx * 2])
				m_laserStates[laserIdx] = -1;
			else if(m_buttonStates[(int32)Button::LS_0Neg + laserIdx * 2 + 1])
				m_laserStates[laserIdx] = 1;
			else
				m_laserStates[laserIdx] = 0;
		}
	}
}

void Input::OnKeyPressed(Key key)
{
	// Handle button mappings
	auto it = m_buttonMap.equal_range(key);
	for(auto it1 = it.first; it1 != it.second; it1++)
		m_OnButtonInput(it1->second, true);
}
void Input::OnKeyReleased(Key key)
{
	// Handle button mappings
	auto it = m_buttonMap.equal_range(key);
	for(auto it1 = it.first; it1 != it.second; it1++)
		m_OnButtonInput(it1->second, false);
}