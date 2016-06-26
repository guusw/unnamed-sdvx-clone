#include "stdafx.h"
#include "HitStat.hpp"

HitStat::HitStat(ObjectState* object) : object(object)
{

}
bool HitStat::operator<(const HitStat& other)
{
	return time < other.time;
}