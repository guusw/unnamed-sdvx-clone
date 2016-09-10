Changing input methods can currently only be done by modifying Main.cfg.
Run the game once to generate a default Main.cfg file

You can change the input method for both Buttons and Lasers by changing the lines:
	LaserInputDevice = Keyboard
	ButtonInputDevice = Keyboard

Supported settings are:
	Controller
	Keyboard
	Mouse (Only for lasers)
	
Change the respective controls:
	Key_*
	Key_*Alt (similar to KB2 settings in KShoot)
	Mouse_*
	Controller_*
	
Both mouse and controller have a sensitivity setting as well that you might want to change depending on your controller or playstyle.

The default mapping for mouse maps Laser0(Blue) to the X-Axis and Laser1(Red) to the Y-Axis of the mouse
	Mouse_Laser0Axis = 0
	Mouse_Laser1Axis = 1
	
If you have more that 1 controller on your system you can change <Controller_DeviceID> to pick the controller that you want.

Controller button and axis ID's (Taken from the way SDL maps controller buttons by default):
Axes:
	SDL_CONTROLLER_AXIS_LEFTX 			= 0,
    SDL_CONTROLLER_AXIS_LEFTY 			= 1,
    SDL_CONTROLLER_AXIS_RIGHTX 			= 2,
    SDL_CONTROLLER_AXIS_RIGHTY 			= 3,
    SDL_CONTROLLER_AXIS_TRIGGERLEFT 	= 4,
    SDL_CONTROLLER_AXIS_TRIGGERRIGHT 	= 5,
Buttons:
	SDL_CONTROLLER_BUTTON_A 			= 0,
    SDL_CONTROLLER_BUTTON_B 			= 1,
    SDL_CONTROLLER_BUTTON_X 			= 2,
    SDL_CONTROLLER_BUTTON_Y 			= 3,
    SDL_CONTROLLER_BUTTON_BACK 			= 4,
    SDL_CONTROLLER_BUTTON_GUIDE 		= 5,
    SDL_CONTROLLER_BUTTON_START 		= 6,
    SDL_CONTROLLER_BUTTON_LEFTSTICK 	= 7,
    SDL_CONTROLLER_BUTTON_RIGHTSTICK 	= 8,
    SDL_CONTROLLER_BUTTON_LEFTSHOULDER 	= 9,
    SDL_CONTROLLER_BUTTON_RIGHTSHOULDER = 10,
    SDL_CONTROLLER_BUTTON_DPAD_UP 		= 11,
    SDL_CONTROLLER_BUTTON_DPAD_DOWN 	= 12,
    SDL_CONTROLLER_BUTTON_DPAD_LEFT 	= 13,
    SDL_CONTROLLER_BUTTON_DPAD_RIGHT 	= 14,