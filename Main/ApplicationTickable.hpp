#pragma once

class IApplicationTickable
{
public:
	virtual ~IApplicationTickable() = default;
	virtual void Tick(float deltaTime) {};
	virtual void Render(float deltaTime) {};
	virtual void OnKeyPressed(Key key) {};
	virtual void OnKeyReleased(Key key) {};
	// Called when focus of this item is lost
	virtual void OnSuspend() {};
	// Called when focus to this item is restored
	virtual void OnRestore() {};
};
