#include "stdafx.h"
#include "GameConfig.hpp"

GameConfig::GameConfig()
{
	// Default state
	Clear();
}

void GameConfig::SetKeyBinding(GameConfigKeys key, Graphics::Key value)
{
	SetEnum<Enum_Key>(key, value);
}

void GameConfig::InitDefaults()
{
	Set(GameConfigKeys::ScreenWidth, 1280);
	Set(GameConfigKeys::ScreenHeight, 720);
	Set(GameConfigKeys::Fullscreen, false);
	Set(GameConfigKeys::FullscreenMonitorIndex, 0);
	Set(GameConfigKeys::ScreenX, -1);
	Set(GameConfigKeys::ScreenY, -1);
	Set(GameConfigKeys::HiSpeed, 1.0f);
	Set(GameConfigKeys::GlobalOffset, 0);
	Set(GameConfigKeys::InputOffset, 0);
	Set(GameConfigKeys::FPSTarget, 0);
	Set(GameConfigKeys::LaserAssistLevel, 2);
	Set(GameConfigKeys::SongFolder, "songs");


	// Input settings
	SetEnum<Enum_InputDevice>(GameConfigKeys::ButtonInputDevice, InputDevice::Keyboard);
	SetEnum<Enum_InputDevice>(GameConfigKeys::LaserInputDevice, InputDevice::Keyboard);

	// Default keyboard bindings
	SetKeyBinding(GameConfigKeys::Key_BT0, Key::S);
	SetKeyBinding(GameConfigKeys::Key_BT1, Key::D);
	SetKeyBinding(GameConfigKeys::Key_BT2, Key::K);
	SetKeyBinding(GameConfigKeys::Key_BT3, Key::L);
	SetKeyBinding(GameConfigKeys::Key_BT0Alt, Key::H);
	SetKeyBinding(GameConfigKeys::Key_BT1Alt, Key::J);
	SetKeyBinding(GameConfigKeys::Key_BT2Alt, Key::F);
	SetKeyBinding(GameConfigKeys::Key_BT3Alt, Key::G);
	SetKeyBinding(GameConfigKeys::Key_FX0, Key::C);
	SetKeyBinding(GameConfigKeys::Key_FX1, Key::M);
	SetKeyBinding(GameConfigKeys::Key_FX0Alt, Key::N);
	SetKeyBinding(GameConfigKeys::Key_FX1Alt, Key::V);
	SetKeyBinding(GameConfigKeys::Key_Laser0Neg, Key::W);
	SetKeyBinding(GameConfigKeys::Key_Laser0Pos, Key::E);
	SetKeyBinding(GameConfigKeys::Key_Laser1Neg, Key::O);
	SetKeyBinding(GameConfigKeys::Key_Laser1Pos, Key::P);

	// Default controller settings
	Set(GameConfigKeys::Controller_DeviceID, 0); // First device
	Set(GameConfigKeys::Controller_BT0, 13); // D-Pad Left
	Set(GameConfigKeys::Controller_BT1, 12); // D-Pad Down
	Set(GameConfigKeys::Controller_BT2, 0); // A / X
	Set(GameConfigKeys::Controller_BT3, 1); // B / O
	Set(GameConfigKeys::Controller_FX0, 9); // Left Shoulder
	Set(GameConfigKeys::Controller_FX1, 10); // Right Shoulder
	Set(GameConfigKeys::Controller_Laser0Axis, 0); // Fist strick Left/Right
	Set(GameConfigKeys::Controller_Laser1Axis, 2); // Second stick Left/Right
	Set(GameConfigKeys::Controller_Sensitivity, 6.0f);
	Set(GameConfigKeys::Controller_Deadzone, 0.1f);

	// Default mouse settings
	Set(GameConfigKeys::Mouse_Laser0Axis, 0);
	Set(GameConfigKeys::Mouse_Laser1Axis, 1);
	Set(GameConfigKeys::Mouse_Sensitivity, 1.0f);
}
