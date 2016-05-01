#include "stdafx.h"
#include "Beatmap.hpp"
#include "KShootMap.hpp"
#include "Profiling.hpp"

bool Beatmap::Load(BinaryStream& input)
{
	ProfilerScope $("Load Beatmap");

	KShootMap kshootMap;
	if(kshootMap.Init(input))
	{
		m_ProcessKShootMap(kshootMap);
	}
	else // Load binary map format
	{
		input.Seek(0);
		if(!m_Serialize(input))
			return false;
	}

	// Precalculate max object durations
	for(auto& os : m_objectStates)
	{
		os.second->maxDuration = os.second->CalculateMaxDuration();
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

Vector<TimingPoint*> Beatmap::GetLinearTimingPoints()
{
	Vector<TimingPoint*> res;
	for(auto& tp : m_timingPoints)
	{
		res.Add(tp.second);
	}
	return res;
}
Vector<ObjectState*> Beatmap::GetLinearObjects()
{
	Vector<ObjectState*> res;
	for(auto& tp : m_objectStates)
	{
		res.Add(tp.second);
	}
	return res;
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
BinaryStream& operator<<(BinaryStream& stream, ObjectState& objState)
{
	// Write Enable states
	if(stream.IsWriting())
	{
		Buffer stateBuffer;
		MemoryWriter stateWriter(stateBuffer);

		uint8 enableBits = 0;
		for(uint32 i = 0; i < 6; i++)
		{
			uint32 mask = 0x1 << i;
			if(objState.buttons[i].on)
			{
				enableBits |= mask;

				stateWriter << (uint32&)objState.buttons[i].duration;
				stateWriter << objState.buttons[i].effectType;
			}
		}
		for(uint32 i = 0; i < 2; i++)
		{
			uint32 mask = 0x40 << i;
			if(objState.lasers[i].duration != 0)
			{
				enableBits |= mask;

				stateWriter << (uint32&)objState.lasers[i].duration;
				stateWriter << objState.lasers[i].points[0];
				stateWriter << objState.lasers[i].points[1];
			}
		}
		stream << enableBits;
		stream.Serialize(stateBuffer.data(), stateBuffer.size());
	}
	else // Read enabled states
	{
		uint8 enableBits = 0;
		stream << enableBits;

		if((enableBits & 0x3F) != 0)
		{
			for(uint32 i = 0; i < 6; i++)
			{
				uint32 mask = 0x1 << i;
				if((enableBits & mask) == mask)
				{
					stream << (uint32&)objState.buttons[i].duration;
					if((uint32&)objState.buttons[i].duration == -1) // Sign extend -1 constant
						objState.buttons[i].duration = -1;
					stream << objState.buttons[i].effectType;
					objState.buttons[i].on = true;
				}
			}
		}
		for(uint32 i = 0; i < 2; i++)
		{
			uint32 mask = 0x40 << i;
			if((enableBits & mask) == mask)
			{
				stream << (uint32&)objState.lasers[i].duration;
				stream << objState.lasers[i].points[0];
				stream << objState.lasers[i].points[1];
			}
		}
	}

	return stream;
}
bool ObjectState::StaticSerialize(BinaryStream& stream, ObjectState*& obj)
{
	if(stream.IsReading())
	{
		obj = new ObjectState();
	}
	stream << *obj;
	return true;
}
bool TimingPoint::StaticSerialize(BinaryStream& stream, TimingPoint*& obj)
{
	if(stream.IsReading())
	{
		obj = new TimingPoint();
	}
	return stream.SerializeObject(*obj);
}
void Beatmap::m_ProcessKShootMap(class KShootMap& kshootMap)
{
	// Process map settings
	for(auto& s : kshootMap.settings)
	{
		if(s.first == "title")
			m_settings.title = Utility::ConvertToUnicode(s.second);
		else if(s.first == "artist")
			m_settings.artist = Utility::ConvertToUnicode(s.second);
		else if(s.first == "effect")
			m_settings.effector = Utility::ConvertToUnicode(s.second);
		else if(s.first == "illustrator")
			m_settings.illustrator = Utility::ConvertToUnicode(s.second);
		else if(s.first == "t")
			m_settings.bpm = Utility::ConvertToUnicode(s.second);
		else if(s.first == "jacket")
			m_settings.jacketPath = s.second;
		else if(s.first == "m")
		{
			if(s.second.find(';') != -1)
			{
				s.second.Split(";", &m_settings.audioNoFX, &m_settings.audioFX);
			}
			else
			{
				m_settings.audioNoFX = s.second;
			}
		}
		else if(s.first == "o")
		{
			m_settings.offset = atoll(*s.second);
		}
	}

	// Process initial timing point
	TimingPoint* lastTimingPoint = new TimingPoint();
	lastTimingPoint->offset = atol(*kshootMap.settings["o"]);
	double bpm = atof(*kshootMap.settings["t"]);
	lastTimingPoint->beatDuration = 60000.0 / bpm;
	lastTimingPoint->measure = 4;
	m_timingPoints.Add(lastTimingPoint->offset, lastTimingPoint);

	MapTime enabledTimes[6]; // Object enable times to detect hold notes and laser durations
	ObjectState* enabledStates[6] = { nullptr };
	MapTime laserEnabledTimes[2];
	ObjectState* laserEnabledStates[2] = { nullptr };

	for(KShootMap::TickIterator it(kshootMap); it; ++it)
	{
		const KShootBlock& block = it.GetCurrentBlock();
		KShootTime time = it.GetTime();
		const KShootTick& tick = *it;

		double blockDuration = (lastTimingPoint->beatDuration * lastTimingPoint->measure);
		MapTime mapTime = MapTime((time.block + (double)time.tick / (double)block.ticks.size()) * blockDuration);

		// Process settings
		for(auto& p : tick.settings)
		{
			if(p.first == "beat")
			{
				// Create new deriving point?
				if(!m_timingPoints.Contains(mapTime))
					lastTimingPoint = new TimingPoint(*lastTimingPoint);

				String n, d;
				if(!p.second.Split("/", &n, &d))
					assert(false);
				uint32 num = atol(*n);
				uint32 denom = atol(*d);
				uint32 measure = num;

				lastTimingPoint->measure = measure;
			}
			else if(p.first == "t")
			{
				// Create new point?
				if(!m_timingPoints.Contains(mapTime))
					lastTimingPoint = new TimingPoint(*lastTimingPoint);

				double bpm = atof(*p.second);
				lastTimingPoint->beatDuration = 60000.0 / bpm;
				m_timingPoints.Add(mapTime, lastTimingPoint);
			}
		}

		// Find or create a new state object
		ObjectState* currentState = nullptr;
		if(m_objectStates.Contains(mapTime))
		{
			currentState = m_objectStates[mapTime];
		}
		else
		{
			currentState = new ObjectState();
			currentState->time = mapTime;
			m_objectStates.Add(mapTime, currentState);
		}

		// Set button states
		for(uint32 i = 0; i < 6; i++)
		{
			char c = i < 4 ? tick.buttons[i] : tick.fx[i - 4];
			if(c == '0')
			{
				// Set hold button duration
				if(enabledStates[i])
				{
					if(enabledStates[i]->buttons[i].duration != -1)
					{
						enabledStates[i]->buttons[i].duration = mapTime - enabledTimes[i];
					}
					enabledStates[i] = nullptr;
				}
			}
			else
			{
				// Store enable of hold button
				if(!enabledStates[i])
				{
					currentState->buttons[i].on = true;
					currentState->buttons[i].effectType = c;
					currentState->buttons[i].duration = -1; // Duration of -1 means instant note
					enabledStates[i] = currentState;
					enabledTimes[i] = mapTime;
				}
				else
				{
					enabledStates[i]->buttons[i].duration = 0;
				}
			}
		}

		// Set laser states
		for(uint32 i = 0; i < 2; i++)
		{
			char c = tick.laser[i];
			if(c == '-')
			{
				if(laserEnabledStates[i])
				{
					// Abort last laser enable
					laserEnabledStates[i]->lasers[i].duration = 0;
					laserEnabledStates[i]->lasers[i].points[1] = laserEnabledStates[i]->lasers[i].points[0];
					laserEnabledStates[i]->lasers[i].next = nullptr;
					if(laserEnabledStates[i]->lasers[i].prev) // Clear next pointer in previous segment
					{
						laserEnabledStates[i]->lasers[i].prev->lasers[i].next = nullptr;
					}
					laserEnabledStates[i]->lasers[i].prev = nullptr;
					laserEnabledStates[i] = nullptr;
				}
			}
			else if(c == ':')
			{
			}
			else
			{
				float pos = kshootMap.TranslateLaserChar(c);
				// Set hold button duration
				if(laserEnabledStates[i])
				{
					laserEnabledStates[i]->lasers[i].duration = mapTime - laserEnabledTimes[i];
					// Set end position of last laser
					laserEnabledStates[i]->lasers[i].points[1] = pos;
					// Set connection in new state
					laserEnabledStates[i]->lasers[i].next = currentState;
					currentState->lasers[i].prev = laserEnabledStates[i];
					laserEnabledStates[i] = nullptr;
				}

				// Set new start
				currentState->lasers[i].points[0] = pos;
				currentState->lasers[i].duration = 1; // Temp duration to make sure this is set
				laserEnabledTimes[i] = mapTime;
				laserEnabledStates[i] = currentState;
			}
		}

		// Discard state object if it does nothing
		if(currentState->IsEmpty())
		{
			m_objectStates.erase(mapTime);
			delete currentState;
		}
	}
}
bool Beatmap::m_Serialize(BinaryStream& stream)
{
	Map<ObjectState*, MapTime> timeMap;
	for(auto& p : m_objectStates)
	{
		timeMap.Add(p.second, p.first);
	}

	stream << m_settings;
	stream << m_timingPoints;
	stream << m_objectStates;

	// Fixup laser next pointers using time
	if(stream.IsReading())
	{
		for(auto& p : m_objectStates)
		{
			p.second->time = p.first;
			for(uint32 i = 0; i < 2; i++)
			{
				LaserState& l = p.second->lasers[i];
				if(l.duration != 0)
				{
					MapTime targetTime = l.duration + p.first;
					if(m_objectStates.Contains(targetTime)) 
					{
						l.next = m_objectStates[targetTime];
						l.next->lasers[i].prev = p.second;
					}
				}
			}
		}
	}

	return true;
}

bool ObjectState::IsEmpty()
{
	for(uint32 i = 0; i < 6; i++)
	{
		if(buttons[i].on)
			return false;
	}
	for(uint32 i = 0; i < 2; i++)
	{
		if(lasers[i].duration != 0)
			return false;
	}
	return true;
}
MapTime ObjectState::CalculateMaxDuration() const
{
	MapTime maxTime = 0;
	for(uint32 i = 0; i < 6; i++)
	{
		if(buttons[i].duration != -1)
			maxTime = Math::Max(buttons[i].duration, maxTime);
	}
	for(uint32 i = 0; i < 2; i++)
	{
		if(lasers[i].duration != 0)
			maxTime = Math::Max(lasers[i].duration, maxTime);
	}
	return maxTime;
}

MapTime TimingPoint::GetSlamTreshold() const
{
	return (MapTime)ceil(beatDuration / (measure*2));
}
void LaserState::Reset(int32 index)
{
	if(index == 0)
	{
		points[0] = 0.0f; points[1] = 1.0f; duration = 0;
	}
	else
	{
		points[0] = 1.0f; points[1] = 0.0f; duration = 0;
	}
}
