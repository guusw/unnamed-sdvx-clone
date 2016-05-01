#include "stdafx.h"
#include "Beatmap.hpp"
#include "KShootMap.hpp"

// Temporary object to keep track if a button is a hold button
struct TempButtonState
{
	TempButtonState(MapTime startTime, uint32 effectType)
		: startTime(startTime), effectType(effectType)
	{
	}
	MapTime startTime;
	uint32 numTicks = 0;
	uint32 effectType = 0;
	uint32 effectParams = 0;
};
struct TempLaserState
{
	TempLaserState(MapTime startTime, uint32 effectType, TimingPoint* tpStart)
		: startTime(startTime), effectType(effectType), tpStart(tpStart)
	{
	}
	// Timing point at which this segment started
	TimingPoint* tpStart;
	MapTime startTime;
	uint32 numTicks = 0;
	uint32 effectType = 0;
	uint32 effectParams = 0;
	float startPosition; // Entry position
	// Previous segment
	LaserObjectState* last = nullptr;
};
bool Beatmap::m_ProcessKShootMap(BinaryStream& input)
{
	KShootMap kshootMap;
	if(!kshootMap.Init(input))
		return false;

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
			m_settings.offset = atol(*s.second);
		}
	}

	// Temporary map for timing points
	Map<MapTime, TimingPoint*> timingPointMap;

	// Process initial timing point
	TimingPoint* lastTimingPoint = new TimingPoint();
	lastTimingPoint->time = atol(*kshootMap.settings["o"]);
	double bpm = atof(*kshootMap.settings["t"]);
	lastTimingPoint->beatDuration = 60000.0 / bpm;
	lastTimingPoint->measure = 4;
	m_timingPoints.Add(lastTimingPoint);
	timingPointMap.Add(lastTimingPoint->time, lastTimingPoint);

	// Button hold states
	TempButtonState* buttonStates[6] = { nullptr };
	// Laser segment states
	TempLaserState* laserStates[2] = { nullptr };

	for(KShootMap::TickIterator it(kshootMap); it; ++it)
	{
		const KShootBlock& block = it.GetCurrentBlock();
		KShootTime time = it.GetTime();
		const KShootTick& tick = *it;

		// Calculate MapTime from current tick
		double blockDuration = (lastTimingPoint->beatDuration * lastTimingPoint->measure);
		MapTime mapTime = MapTime((time.block + (double)time.tick / (double)block.ticks.size()) * blockDuration);

		// Process settings
		for(auto& p : tick.settings)
		{
			if(p.first == "beat")
			{
				// Create new deriving point?
				if(!timingPointMap.Contains(mapTime))
				{
					lastTimingPoint = new TimingPoint(*lastTimingPoint);
					m_timingPoints.Add(lastTimingPoint);
					timingPointMap.Add(mapTime, lastTimingPoint);
				}

				String n, d;
				if(!p.second.Split("/", &n, &d))
					assert(false);
				uint32 num = atol(*n);
				uint32 denom = atol(*d);

				lastTimingPoint->measure = num;
			}
			else if(p.first == "t")
			{
				// Create new point?
				if(!timingPointMap.Contains(mapTime))
				{
					lastTimingPoint = new TimingPoint(*lastTimingPoint);
					m_timingPoints.Add(lastTimingPoint);
					timingPointMap.Add(mapTime, lastTimingPoint);
				}

				double bpm = atof(*p.second);
				lastTimingPoint->beatDuration = 60000.0 / bpm;
			}
		}

		// Set button states
		for(uint32 i = 0; i < 6; i++)
		{
			char c = i < 4 ? tick.buttons[i] : tick.fx[i - 4];
			TempButtonState*& state = buttonStates[i];
			if(c == '0')
			{
				// Terminate hold button
				if(state)
				{
					if(state->numTicks > 0)
					{
						HoldObjectState* obj = new HoldObjectState();
						obj->time = state->startTime;
						obj->index = i;
						obj->duration = mapTime - state->startTime;
						obj->effectType = (uint8)state->effectType;
						m_objectStates.Add(*obj);
					}
					else
					{
						ButtonObjectState* obj = new ButtonObjectState();
						obj->time = state->startTime;
						obj->index = i;
						m_objectStates.Add(*obj);
					}

					// Reset 
					delete state;
					state = nullptr;
				}
			}
			else if(!state)
			{
				// Create new hold state
				state = new TempButtonState(mapTime, (uint32)c);
			}
			else
			{
				// Update current hold state
				state->numTicks++;
			}
		}

		// Set laser states
		for(uint32 i = 0; i < 2; i++)
		{
			TempLaserState*& state = laserStates[i];
			char c = tick.laser[i];

			// Function that creates a new segment out of the current state
			auto CreateLaserSegment = [&](float endPos) 
			{
				// Process existing segment
				assert(state->numTicks > 0);

				LaserObjectState* obj = new LaserObjectState();
				obj->time = state->startTime;
				obj->duration = mapTime - state->startTime;
				obj->index = i;
				obj->points[0] = state->startPosition;
				obj->points[1] = endPos;

				// Threshold for laser segments to be considered instant
				MapTime laserSlamThreshold = (MapTime)ceil(state->tpStart->beatDuration / 8.0);
				if(obj->duration <= laserSlamThreshold)
					obj->flags |= LaserObjectState::flag_Instant;

				// Link segments together
				if(state->last)
				{
					obj->prev = state->last;
					obj->prev->next = obj;
				}

				// Add to list of objects
				m_objectStates.Add(*obj);

				return obj;
			};

			if(c == '-')
			{
				// Terminate laser
				if(state)
				{
					// Reset state
					delete state;
					state = nullptr;
				}
			}
			else if(c == ':')
			{
				// Update current laser state
				if(state)
				{
					state->numTicks++;
				}
			}
			else
			{
				float pos = kshootMap.TranslateLaserChar(c);
				LaserObjectState* last = nullptr;
				if(state)
				{
					last = CreateLaserSegment(pos);

					// Reset state
					delete state;
					state = nullptr;
				}

				MapTime startTime = mapTime;
				if(last && (last->flags & LaserObjectState::flag_Instant) != 0)
				{
					// Move offset to be the same as last segment, as in ksh maps there is a 1 tick delay after laser slams
					startTime = last->time;
				}
				state = new TempLaserState(startTime, 0, lastTimingPoint);
				state->last = last; // Link together
				state->startPosition = pos;
			}
		}
	}

	// Re-sort collection to fix some inconsistencies caused by corrections after laser slams
	m_objectStates.Sort([](const ObjectState* l, const ObjectState* r) {
		return l->time < r->time;
	});

	return true;
}