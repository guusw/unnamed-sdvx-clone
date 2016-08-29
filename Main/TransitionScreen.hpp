#pragma once
#include "ApplicationTickable.hpp"
#include "AsyncLoadable.hpp"

/*
	Loading and transition screen
*/
class TransitionScreen : public IApplicationTickable
{
public:
	virtual ~TransitionScreen() = default;
	static TransitionScreen* Create(IAsyncLoadableApplicationTickable* next);

	// Called when target screen is loaded
	//	can also give a null pointer if the screen didn't load successfully
	Delegate<IAsyncLoadableApplicationTickable*> OnLoadingComplete;
};