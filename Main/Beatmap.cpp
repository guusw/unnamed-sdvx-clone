#include "stdafx.h"
#include "Beatmap.hpp"
#include "Profiling.hpp"

static const uint32 c_mapVersion = 1;

bool Beatmap::Load(BinaryStream& input)
{
	ProfilerScope $("Load Beatmap");

	if(!m_ProcessKShootMap(input)) // Load KSH format first
	{
		// Load binary map format
		input.Seek(0);
		if(!m_Serialize(input))
			return false;
	}

	return true;
}
bool Beatmap::Save(BinaryStream& output)
{
	ProfilerScope $("Save Beatmap");
	return m_Serialize(output);
}

const BeatmapSettings& Beatmap::GetMapSettings()
{
	return m_settings;
}

Vector<TimingPoint*>& Beatmap::GetLinearTimingPoints()
{
	return m_timingPoints;
}
Vector<ObjectState*>& Beatmap::GetLinearObjects()
{
	return reinterpret_cast<Vector<ObjectState*>&>(m_objectStates);
}

bool MultiObjectState::StaticSerialize(BinaryStream& stream, MultiObjectState*& obj)
{
	uint8 type = 0;
	if(stream.IsReading())
	{
		// Read type and create appropriate object
		stream << type;
		switch((ObjectType)type)
		{
		case ObjectType::Single:
			obj = (MultiObjectState*)new ButtonObjectState();
			break;
		case ObjectType::Hold:
			obj = (MultiObjectState*)new HoldObjectState();
			break;
		case ObjectType::Laser:
			obj = (MultiObjectState*)new LaserObjectState();
			break;
		case ObjectType::Event:
			obj = (MultiObjectState*)new EventObjectState();
			break;
		}
	}
	else
	{
		// Write type
		type = (uint8)obj->type;
		stream << type;
	}

	// Pointer is always initialized here, serialize data
	stream << obj->time; // Time always set
	switch(obj->type)
	{
	case ObjectType::Single:
		stream << obj->button.index;
		break;
	case ObjectType::Hold:
		stream << obj->hold.index;
		stream << obj->hold.duration;
		stream << (uint8&)obj->hold.effectType;
		stream << (uint8&)obj->hold.effectParam;
		break;
	case ObjectType::Laser:
		stream << obj->laser.index;
		stream << obj->laser.duration;
		stream << obj->laser.points[0];
		stream << obj->laser.points[1];
		stream << obj->laser.flags;
		break;
	case ObjectType::Event:
		stream << (uint8&)obj->event.key;
		stream << (uint32&)obj->event.intVal;
		break;
	}

	return true;
}
bool TimingPoint::StaticSerialize(BinaryStream& stream, TimingPoint*& out)
{
	if(stream.IsReading())
		out = new TimingPoint();
	stream << out->time;
	stream << out->beatDuration;
	stream << out->measure;
	return true;
}

BinaryStream& operator<<(BinaryStream& stream, BeatmapSettings& settings)
{
	stream << settings.title;
	stream << settings.artist;
	stream << settings.effector;
	stream << settings.tags;
	stream << settings.bpm;
	stream << settings.offset;
	stream << settings.audioNoFX;
	stream << settings.audioFX;
	stream << settings.jacketPath;
	return stream;
}
bool Beatmap::m_Serialize(BinaryStream& stream)
{
	static const uint32 c_magic = *(uint32*)"FXMM";
	uint32 magic = c_magic;
	uint32 version = c_mapVersion;
	stream << magic;
	stream << version;

	// Validate headers when reading
	if(stream.IsReading())
	{
		if(magic != c_magic)
		{
			Log("Invalid map format", Logger::Warning);
			return false;
		}
		if(version != c_mapVersion)
		{
			Logf("Incompatible map version [%d], loader is version %d", Logger::Warning, version, c_mapVersion);
			return false;
		}
	}

	stream << m_settings;
	stream << m_timingPoints;
	stream << reinterpret_cast<Vector<MultiObjectState*>&>(m_objectStates);

	// Manually fix up laser next-prev pointers
	LaserObjectState* prevLasers[2] = { 0 };
	if(stream.IsReading())
	{
		for(ObjectState* obj : m_objectStates)
		{
			if(obj->type == ObjectType::Laser)
			{
				LaserObjectState* laser = (LaserObjectState*)obj;
				LaserObjectState*& prev = prevLasers[laser->index];
				if(prev && (prev->time + prev->duration) == laser->time)
				{
					prev->next = laser;
					laser->prev = prev;
				}

				prev = laser;
			}
		}
	}

	return true;
}

