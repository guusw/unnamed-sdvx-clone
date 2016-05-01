#include "stdafx.h"
#include "Beatmap.hpp"
#include "Profiling.hpp"

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
	return m_objectStates;
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
	/*
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
	*/
	return true;
}