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

If you want to find out the button/axis id's for your controller 
press F8 while playing a map with the correct controller ID set to see the buttons 
on the right side which are pressed.