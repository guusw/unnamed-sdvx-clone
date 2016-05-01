#pragma once

class IApplicationTickable
{
public:
	virtual void Tick(float deltaTime) {};
	virtual void Render(float deltaTime) {};
	virtual void OnKeyPressed(uint8 key) {};
	virtual void OnKeyReleased(uint8 key) {};
};
