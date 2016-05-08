/*
	This file contains all the data types that are used for objects inside maps
	The basic object class is ObjectState
		this class contains 'type' member which indicates to which type it is castable

	No vtable for smaller memory footprint
*/
#pragma once
// For effect type enum
#include "AudioEffects.hpp"

// Time unit used for all objects
// this is the offset from the audio file beginning in ms for timing points
// this is the offset from the map's global offset in ms for object states
// this type is also used for negative time delta values
// the maximum map length that can be represented by map time is 
//	2147483648	ms
//	~2147483	sec
//	~35791		min
typedef int32 MapTime;

// Type enum for map object
enum class ObjectType : uint8
{
	Invalid = 0,
	Single, // Either normal or FX button
	Hold, // Either normal or FX button but with a duration
	Laser, // A laser segment
	Event // Event object
};

// The key parameter for event objects
enum class EventKey : uint8
{
	SlamVolume, // Float
	LaserEffectType, // uint8
	LaserEffectMix, // Float
	TrackRollBehaviour, // uint8
};

// Common data for all object types
struct ObjectTypeData_Base
{
	ObjectTypeData_Base(ObjectType type) : type(type) {};

	// Position in ms when this object appears
	MapTime time;
	// Type of this object, determines the size of this struct and which type its data is
	ObjectType type;
};

struct MultiObjectState;

// Object state type with contains specific data of type T
template<typename T>
struct TObjectState : public ObjectTypeData_Base, T
{
	TObjectState() : ObjectTypeData_Base(T::staticType) {};

	// Implicit down-cast
	operator TObjectState<void>*() { return (TObjectState<void>*)this; }
	operator const TObjectState<void>*() const { return (TObjectState<void>*)this; }
	// Implicit down-cast
	operator MultiObjectState*() { return (MultiObjectState*)this; }
	operator const MultiObjectState*() const { return (MultiObjectState*)this; }
};

// Generic object, does not have an object member
template<> struct TObjectState<void> : public ObjectTypeData_Base
{
	TObjectState() : ObjectTypeData_Base(ObjectType::Invalid) {};

	// Always allow casting from typeless object to Union State object
	operator MultiObjectState*() { return (MultiObjectState*)this; }
	operator const MultiObjectState*() const { return (MultiObjectState*)this; }
};

// A Single button
struct ObjectTypeData_Button
{
	// The index of the button
	// 0-3 Normal buttons
	// 4-5 FX buttons
	uint8 index = 0xFF;

	static const ObjectType staticType = ObjectType::Single;
};
// A Hold button, extends a normal button with duration and effect type
struct ObjectTypeData_Hold : public ObjectTypeData_Button
{
	// Used for hold notes, 0 is a normal note
	MapTime duration = 0;
	// The sound effect on the note
	EffectType effectType = EffectType::None;
	// The parameter for effects that have it
	EffectParam effectParam = 0;

	static const ObjectType staticType = ObjectType::Hold;
};

// A laser segment
struct ObjectTypeData_Laser
{
	// Duration of laser segment
	MapTime duration = 0;
	// 0 or 1 for left and right respectively
	uint8 index = 0;
	// Special options
	uint8 flags = 0;
	// Position of the laser on the track
	float points[2];
	// Set the to the object state that connects to this laser, if any, otherwise null
	TObjectState<ObjectTypeData_Laser>* next = nullptr;
	TObjectState<ObjectTypeData_Laser>* prev = nullptr;

	static const ObjectType staticType = ObjectType::Laser;

	// Indicates that this segment is instant and should generate a laser slam segment
	static const uint8 flag_Instant = 0x1;
	// Indicates that the range of this laser is extended from -0.5 to 1.5
	static const uint8 flag_Extended = 0x2;
};

// An event segment, these set various settings at a given point, such as an effect volume, the roll behaviour of the track, etc.
struct ObjectTypeData_Event
{
	// The key value for what value this event is setting
	EventKey key;

	// Always 32 bits of data, but the one used depends on the key
	union 
	{
		float floatVal;
		int32 intVal;
		uint8 byteVal;
		LaserEffectType effectVal;
	};

	static const ObjectType staticType = ObjectType::Event;
};

// Object state with union data member
struct MultiObjectState
{
	static bool StaticSerialize(BinaryStream& stream, MultiObjectState*& out);

	// Position in ms when this object appears
	MapTime time;
	// Type of this object, determines the size of this struct and which type its data is
	ObjectType type;
	union 
	{
		ObjectTypeData_Button button;
		ObjectTypeData_Hold hold;
		ObjectTypeData_Laser laser;
		ObjectTypeData_Event event;
	};

	// Implicit down-cast
	operator TObjectState<void>*() { return (TObjectState<void>*)this; }
	operator const TObjectState<void>*() const { return (TObjectState<void>*)this; }
};

typedef TObjectState<void> ObjectState;
typedef TObjectState<ObjectTypeData_Button> ButtonObjectState;
typedef TObjectState<ObjectTypeData_Hold> HoldObjectState;
typedef TObjectState<ObjectTypeData_Laser> LaserObjectState;
typedef TObjectState<ObjectTypeData_Event> EventObjectState;

// Map timing point
struct TimingPoint
{
	static bool StaticSerialize(BinaryStream& stream, TimingPoint*& out);

	// Beat duration in milliseconds
	//	this is a double so the least precision is lost
	//	can be cast back to integer format once is has been multiplied by the amount of beats you want the length of.
	// Calculated by taking (60000.0 / BPM)
	double beatDuration;
	// This is just set to the numerator of the time signature
	// 4/4 -> 4
	// 3/4 -> 3
	// 8/8 -> 8
	uint8 measure;
	// Position in ms when this timing point appears
	MapTime time;
};