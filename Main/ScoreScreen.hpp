#pragma once
#include "ApplicationTickable.hpp"
#include "AsyncLoadable.hpp"

class ScoreScreen : public IAsyncLoadableApplicationTickable
{
public:
	virtual ~ScoreScreen() = default;
	static ScoreScreen* Create(class Game* game);
};