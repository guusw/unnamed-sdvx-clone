#include "stdafx.h"
#include "Beatmap.hpp"
#include "Profiling.hpp"

static const uint32 c_mapVersion = 1;

Beatmap::~Beatmap()
{
	// Perform cleanup
	for(auto tp : m_timingPoints)
		delete tp;
	for(auto obj : m_objectStates)
		delete obj;
	for(auto z : m_zoomControlPoints)
		delete z;
}

bool Beatmap::Load(BinaryStream& input, bool metadataOnly)
{
	ProfilerScope $("Load Beatmap");

	if(!m_ProcessKShootMap(input, metadataOnly)) // Load KSH format first
	{
		// Load binary map format
		input.Seek(0);
		if(!m_Serialize(input, metadataOnly))
			return false;
	}

	return true;
}
bool Beatmap::Save(BinaryStream& output)
{
	ProfilerScope $("Save Beatmap");
	return m_Serialize(output, false);
}

const BeatmapSettings& Beatmap::GetMapSettings()
{
	return m_settings;
}

const Vector<TimingPoint*>& Beatmap::GetLinearTimingPoints() const
{
	return m_timingPoints;
}
const Vector<ObjectState*>& Beatmap::GetLinearObjects() const
{
	return reinterpret_cast<const Vector<ObjectState*>&>(m_objectStates);
}
const Vector<ZoomControlPoint*>& Beatmap::GetZoomControlPoints() const
{
	return m_zoomControlPoints;
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
		stream << *&obj->event.data;
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
	stream << out->numerator;
	return true;
}

BinaryStream& operator<<(BinaryStream& stream, BeatmapSettings& settings)
{
	stream << settings.title;
	stream << settings.artist;
	stream << settings.effector;
	stream << settings.illustrator;
	stream << settings.tags;

	stream << settings.bpm;
	stream << settings.offset;

	stream << settings.audioNoFX;
	stream << settings.audioFX;

	stream << settings.jacketPath;

	stream << settings.level;
	stream << settings.difficulty;

	stream << settings.previewOffset;
	stream << settings.previewDuration;

	stream << settings.slamVolume;
	stream << settings.laserEffectMix;
	stream << (uint8&)settings.laserEffectType;
	return stream;
}
bool Beatmap::m_Serialize(BinaryStream& stream, bool metadataOnly)
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

// Enum OR, AND
TrackRollBehaviour operator|(const TrackRollBehaviour& l, const TrackRollBehaviour& r)
{
	return (TrackRollBehaviour)((uint8)l | (uint8)r);
}
TrackRollBehaviour operator&(const TrackRollBehaviour& l, const TrackRollBehaviour& r)
{
	return (TrackRollBehaviour)((uint8)l & (uint8)r);
}

bool BeatmapSettings::StaticSerialize(BinaryStream& stream, BeatmapSettings*& settings)
{
	if(stream.IsReading())
		settings = new BeatmapSettings();
	stream << *settings;
	return true;
}
