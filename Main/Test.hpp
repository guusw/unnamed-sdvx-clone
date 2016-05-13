#pragma once
#include "ApplicationTickable.hpp"

/* 
	Simple test scene that can be freely used when developing new graphics features 
*/
class Test : public IApplicationTickable
{
protected:
	Test() = default;
public:
	virtual ~Test() = default;
	static Test* Create();
};