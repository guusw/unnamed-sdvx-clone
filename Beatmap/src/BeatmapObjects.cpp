#include "stdafx.h"
#include "BeatmapObjects.hpp"

// Object array sorting
void TObjectState<void>::SortArray(Vector<ObjectState*>& arr)
{
	arr.Sort([](const ObjectState* l, const ObjectState* r)
	{
		if(l->time == r->time)
		{
			// Sort laser slams to come first
			bool ls = l->type == ObjectType::Laser && (((LaserObjectState*)l)->flags & LaserObjectState::flag_Instant);
			bool rs = r->type == ObjectType::Laser && (((LaserObjectState*)r)->flags & LaserObjectState::flag_Instant);
			return ls > rs;
		}
		return l->time < r->time;
	});
}
TObjectState<ObjectTypeData_Hold>* ObjectTypeData_Hold::GetRoot()
{
	TObjectState<ObjectTypeData_Hold>* ptr = (TObjectState<ObjectTypeData_Hold>*)this;
	while(ptr->prev)
		ptr = ptr->prev;
	return ptr;
}

TObjectState<ObjectTypeData_Laser>* ObjectTypeData_Laser::GetRoot()
{
	TObjectState<ObjectTypeData_Laser>* ptr = (TObjectState<ObjectTypeData_Laser>*)this;
	while(ptr->prev)
		ptr = ptr->prev;
	return ptr;
}
TObjectState<ObjectTypeData_Laser>* ObjectTypeData_Laser::GetTail()
{
	TObjectState<ObjectTypeData_Laser>* ptr = (TObjectState<ObjectTypeData_Laser>*)this;
	while(ptr->next)
		ptr = ptr->next;
	return ptr;
}
float ObjectTypeData_Laser::GetDirection() const
{
	return Math::Sign(points[1] - points[0]);
}
float ObjectTypeData_Laser::SamplePosition(MapTime time) const
{
	const LaserObjectState* state = (LaserObjectState*)this;
	while(state->next && (state->time + state->duration) < time)
	{
		state = state->next;
	}
	float f = Math::Clamp((float)(time - state->time) / (float)Math::Max(1, state->duration), 0.0f, 1.0f);
	return (state->points[1] - state->points[0]) * f + state->points[0];
}

float ObjectTypeData_Laser::ConvertToNormalRange(float inputRange)
{
	return (inputRange + 0.5f) * 0.5f;
}
float ObjectTypeData_Laser::ConvertToExtendedRange(float inputRange)
{
	return inputRange * 2.0f - 0.5f;
}

// Enum OR, AND
TrackRollBehaviour operator|(const TrackRollBehaviour& l, const TrackRollBehaviour& r)
{
	return (TrackRollBehaviour)((uint8)l | (uint8)r);
}
TrackRollBehaviour operator&(const TrackRollBehaviour& l, const TrackRollBehaviour& r)
{
	return (TrackRollBehaviour)((uint8)l & (uint8)r);
}