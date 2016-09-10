#pragma once
#include "Shared/Config.hpp"
#include "Input.hpp"

DefineEnum(GameConfigKeys,
	// Screen settings
	ScreenWidth,
	ScreenHeight,
	ScreenX,
	ScreenY,
	Fullscreen,
	FullscreenMonitorIndex,

	// Game settings
	HiSpeed,
	GlobalOffset,
	SongFolder,

	// Input device setting per element
	LaserInputDevice,
	ButtonInputDevice,

	// Mouse settings (primary axes are x=0, y=1)
	Mouse_Laser0Axis,
	Mouse_Laser1Axis,
	Mouse_Sensitivity,

	// Key bindings
	Key_BT0,
	Key_BT1,
	Key_BT2,
	Key_BT3,
	Key_BT0Alt,
	Key_BT1Alt,
	Key_BT2Alt,
	Key_BT3Alt,
	Key_FX0,
	Key_FX1,
	Key_FX0Alt,
	Key_FX1Alt,
	Key_Laser0Pos,
	Key_Laser0Neg,
	Key_Laser1Pos,
	Key_Laser1Neg,

	// Controller bindings
	Controller_DeviceID,
	Controller_BT0,
	Controller_BT1,
	Controller_BT2,
	Controller_BT3,
	Controller_FX0,
	Controller_FX1,
	Controller_Laser0Axis,
	Controller_Laser1Axis,
	Controller_Deadzone,
	Controller_Sensitivity
	);

// Config for game settings
class GameConfig : public Config<Enum_GameConfigKeys>
{
public:
	GameConfig();
	void SetKeyBinding(GameConfigKeys key, Graphics::Key value);

protected:
	virtual void InitDefaults() override;

};

// Main config instance
extern class GameConfig g_gameConfig;