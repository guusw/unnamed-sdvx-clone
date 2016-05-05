#pragma once

class Background
{
public:
	virtual ~Background() = default;
	virtual bool Init() = 0;
	virtual void Render(float deltaTime) = 0;

	class Game* game;
};

Background* CreateBackground(class Game* game);