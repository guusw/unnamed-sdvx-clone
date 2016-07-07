#include "stdafx.h"
#include "Beatmap.hpp"
#include "KShootMap.hpp"

// Temporary object to keep track if a button is a hold button
struct TempButtonState
{
	TempButtonState(MapTime startTime)
		: startTime(startTime)
	{
	}
	MapTime startTime;
	uint32 numTicks = 0;
	EffectType effectType = EffectType::None;
	EffectParam effectParams = 0;
	// If using the smalles grid to indicate hold note duration
	bool fineSnap = false;
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
bool Beatmap::m_ProcessKShootMap(BinaryStream& input, bool metadataOnly)
{
	KShootMap kshootMap;
	if(!kshootMap.Init(input, metadataOnly))
		return false;

	auto ParseFilterType = [](const String& str)
	{
		LaserEffectType type = LaserEffectType::None;
		if(str == "hpf1")
		{
			type = LaserEffectType::HighPassFilter;
		}
		else if(str == "lpf1")
		{
			type = LaserEffectType::LowPassFilter;
		}
		else if(str == "fx;bitc")
		{
			type = LaserEffectType::Bitcrush;
		}
		else if(str == "peak")
		{
			type = LaserEffectType::PeakingFilter;
		}
		return type;
	};

	// Process map settings
	for(auto& s : kshootMap.settings)
	{
		if(s.first == "title")
			m_settings.title = s.second;
		else if(s.first == "artist")
			m_settings.artist = s.second;
		else if(s.first == "effect")
			m_settings.effector = s.second;
		else if(s.first == "illustrator")
			m_settings.illustrator = s.second;
		else if(s.first == "t")
			m_settings.bpm = s.second;
		else if(s.first == "jacket")
			m_settings.jacketPath = s.second;
		else if(s.first == "m")
		{
			if(s.second.find(';') != -1)
			{
				String audioFX, audioNoFX;
				s.second.Split(";", &audioNoFX, &audioFX);
				m_settings.audioFX = audioFX;
				m_settings.audioNoFX = audioNoFX;
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
		else if(s.first == "filtertype")
		{
			m_settings.laserEffectType = ParseFilterType(s.second);
		}
		else if(s.first == "pfiltergain")
		{
			m_settings.laserEffectMix = (float)atol(*s.second) / 100.0f;
		}
		else if(s.first == "chokkakuvol")
		{
			m_settings.slamVolume = (float)atol(*s.second) / 100.0f;
		}
	}

	if(metadataOnly)
		return true;

	// Temporary map for timing points
	Map<MapTime, TimingPoint*> timingPointMap;

	// Process initial timing point
	TimingPoint* lastTimingPoint = new TimingPoint();
	lastTimingPoint->time = atol(*kshootMap.settings["o"]);
	double bpm = atof(*kshootMap.settings["t"]);
	lastTimingPoint->beatDuration = 60000.0 / bpm;
	lastTimingPoint->numerator = 4;

	// Block offset for current timing point
	uint32 timingPointBlockOffset = 0;
	// Tick offset into block for current timing point
	uint32 timingTickOffset = 0;
	// Duration of first timing block
	double timingFirstBlockDuration = 0.0f;

	// Ending time of last timing point
	m_timingPoints.Add(lastTimingPoint);
	timingPointMap.Add(lastTimingPoint->time, lastTimingPoint);

	// Button hold states
	TempButtonState* buttonStates[6] = { nullptr };
	// Laser segment states
	TempLaserState* laserStates[2] = { nullptr };

	EffectParam effectParams[2] = { 0 };
	float laserRanges[2] = { 1.0f, 1.0f };

	for(KShootMap::TickIterator it(kshootMap); it; ++it)
	{
		const KShootBlock& block = it.GetCurrentBlock();
		KShootTime time = it.GetTime();
		const KShootTick& tick = *it;

		// Calculate MapTime from current tick
		double blockDuration = lastTimingPoint->GetBarDuration();
		uint32 blockFromStartOfTimingPoint = (time.block - timingPointBlockOffset);
		uint32 tickFromStartOfTimingPoint;

		if(blockFromStartOfTimingPoint == 0) // Use tick offset when in first block
			tickFromStartOfTimingPoint = (time.tick - timingTickOffset);
		else
			tickFromStartOfTimingPoint = time.tick;

		// Get the offset calculated by adding block durations together
		double blockDurationOffset = 0;
		if(timingTickOffset > 0) // First block might have a shorter length because of the timing point being mid tick
		{
			if(blockFromStartOfTimingPoint > 0)
				blockDurationOffset = timingFirstBlockDuration + blockDuration * (blockFromStartOfTimingPoint - 1);
		}
		else
		{
			blockDurationOffset = blockDuration * blockFromStartOfTimingPoint;
		}

		// Sub-Block offset by adding ticks together
		double blockPercent = (double)tickFromStartOfTimingPoint / (double)block.ticks.size();
		double tickOffset = blockPercent * blockDuration;
		MapTime mapTime = lastTimingPoint->time + MapTime(blockDurationOffset + tickOffset);

		bool lastTick = &block == &kshootMap.blocks.back() &&
			&tick == &block.ticks.back();

		// Process settings
		for(auto& p : tick.settings)
		{
			// Functions that adds a new timing point at current location if it's not yet there
			auto AddTimingPoint = [&](double newDuration, uint32 newNum, uint32 newDenom)
			{
				// Does not yet exist at current time?
				if(!timingPointMap.Contains(mapTime))
				{
					lastTimingPoint = new TimingPoint(*lastTimingPoint);
					lastTimingPoint->time = mapTime;
					m_timingPoints.Add(lastTimingPoint);
					timingPointMap.Add(mapTime, lastTimingPoint);
					timingPointBlockOffset = time.block;
					timingTickOffset = time.tick;
				}

				lastTimingPoint->numerator = newNum;
				lastTimingPoint->denominator = newDenom;
				lastTimingPoint->beatDuration = newDuration;

				// Calculate new block duration
				blockDuration = lastTimingPoint->GetBarDuration();

				// Set new first block duration based on remaining ticks
				timingFirstBlockDuration = (double)(block.ticks.size() - time.tick) / (double)block.ticks.size() * blockDuration;
			};

			if(p.first == "beat")
			{
				String n, d;
				if(!p.second.Split("/", &n, &d))
					assert(false);
				uint32 num = atol(*n);
				uint32 denom = atol(*d);
				assert(denom % 4 == 0);

				AddTimingPoint(lastTimingPoint->beatDuration, num, denom);
			}
			else if(p.first == "t")
			{
				double bpm = atof(*p.second);
				AddTimingPoint(60000.0 / bpm, lastTimingPoint->numerator, lastTimingPoint->denominator);
			}
			else if(p.first == "laserrange_l")
			{
				laserRanges[0] = 2.0f;
			}
			else if(p.first == "laserrange_r")
			{
				laserRanges[1] = 2.0f;
			}
			else if(p.first == "fx-l_param1")
			{
				effectParams[0] = (EffectParam)atol(*p.second);
			}
			else if(p.first == "fx-r_param1")
			{
				effectParams[1] = (EffectParam)atol(*p.second);
			}
			else if(p.first == "filtertype")
			{
				// Inser filter type change event
				EventObjectState* evt = new EventObjectState();
				evt->time = mapTime;
				evt->key = EventKey::LaserEffectType;
				evt->data.effectVal = ParseFilterType(p.second);
				m_objectStates.Add(*evt);
			}
			else if(p.first == "pfiltergain")
			{
				// Inser filter type change event
				float gain = (float)atol(*p.second) / 100.0f;
				EventObjectState* evt = new EventObjectState();
				evt->time = mapTime;
				evt->key = EventKey::LaserEffectMix;
				evt->data.floatVal = gain;
				m_objectStates.Add(*evt);
			}
			else if(p.first == "chokkakuvol")
			{
				float vol = (float)atol(*p.second) / 100.0f;
				EventObjectState* evt = new EventObjectState();
				evt->time = mapTime;
				evt->key = EventKey::LaserEffectMix;
				evt->data.floatVal = vol;
				m_objectStates.Add(*evt);
			}
			else if(p.first == "zoom_bottom")
			{
				ZoomControlPoint* point = new ZoomControlPoint();
				point->time = mapTime;
				point->index = 0;
				point->zoom = (float)atol(*p.second) / 100.0f;
				m_zoomControlPoints.Add(point);
			}
			else if(p.first == "zoom_top")
			{
				ZoomControlPoint* point = new ZoomControlPoint();
				point->time = mapTime;
				point->index = 1;
				point->zoom = (float)atol(*p.second) / 100.0f;
				m_zoomControlPoints.Add(point);
			}
			else if(p.first == "tilt")
			{
				EventObjectState* evt = new EventObjectState();
				evt->time = mapTime;
				evt->key = EventKey::TrackRollBehaviour;
				evt->data.rollVal = TrackRollBehaviour::Zero;
				String v = p.second;
				size_t f = v.find("keep_");
				if(f != -1)
				{
					evt->data.rollVal = TrackRollBehaviour::Keep;
					v = v.substr(f + 5);
				}

				if(v == "normal")
				{
					evt->data.rollVal = evt->data.rollVal | TrackRollBehaviour::Normal;
				}
				else if(v == "bigger")
				{
					evt->data.rollVal = evt->data.rollVal | TrackRollBehaviour::Bigger;
				}
				else if(v == "biggest")
				{
					evt->data.rollVal = evt->data.rollVal | TrackRollBehaviour::Biggest;
				}

				m_objectStates.Add(*evt);
			}
		}

		// Set button states
		for(uint32 i = 0; i < 6; i++)
		{
			char c = i < 4 ? tick.buttons[i] : tick.fx[i - 4];
			TempButtonState*& state = buttonStates[i];

			auto CreateButton = [&]()
			{
				if(state->numTicks > 0 && state->fineSnap)
				{
					HoldObjectState* obj = new HoldObjectState();
					obj->time = state->startTime;
					obj->index = i;
					obj->duration = mapTime - state->startTime;
					obj->effectType = state->effectType;
					obj->effectParam = state->effectParams;
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
			};

			if(c == '0')
			{
				// Terminate hold button
				if(state)
				{
					CreateButton();
				}

				if(i >= 4)
				{
					// Unset effect parameters
					effectParams[i-4] = 0;
				}
			}
			else if(!state)
			{
				// Create new hold state
				state = new TempButtonState(mapTime);
				uint32 div = (uint32)block.ticks.size();

				if(i < 4)
				{
					// Normal '1' notes are always individual
					state->fineSnap = c != '1';
				}
				else
				{
					// Hold are always on a high enough snap to make suere they are seperate when needed
					state->fineSnap = true;

					// Set effect
					if(c == 'B')
					{
						state->effectType = EffectType::Bitcrush;
						state->effectParams = effectParams[i-4];
					}
					else if(c >= 'G' && c <= 'L') // Gate 4/8/16/32/12/24
					{
						state->effectType = EffectType::Gate;
						EffectParam paramMap[] = {
							4, 8, 16, 32, 12, 24
						};
						state->effectParams = paramMap[c - 'G'];
					}
					else if(c >= 'S' && c <= 'W') // Retrigger 8/16/32/12/24
					{
						state->effectType = EffectType::Retrigger;
						EffectParam paramMap[] = {
							8, 16, 32, 12, 24
						};
						state->effectParams = paramMap[c - 'S'];
					}
					else if(c == 'Q')
					{
						state->effectType = EffectType::Phaser;
					}
					else if(c == 'F')
					{
						state->effectType = EffectType::Flanger;
					}
					else if(c == 'X')
					{
						state->effectType = EffectType::Wobble;
						state->effectParams = 12;
					}
					else if(c == 'D')
					{
						state->effectType = EffectType::SideChain;
					}
					else if(c == 'A')
					{
						state->effectType = EffectType::TapeStop;
						state->effectParams = effectParams[i-4];
					}
				}
			}
			else
			{
				// For buttons not using the 1/32 grid
				if(!state->fineSnap)
				{
					CreateButton();

					// Create new hold state
					state = new TempButtonState(mapTime);
					uint32 div = (uint32)block.ticks.size();
					
					if(i < 4)
					{
						// Normal '1' notes are always individual
						state->fineSnap = c != '1';
					}
					else
					{
						// Hold are always on a high enough snap to make suere they are seperate when needed
						state->fineSnap = true;
					}
				}
				else
				{
					// Update current hold state
					state->numTicks++;
				}
			}

			// Terminate last item
			if(lastTick && state)
				CreateButton();
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
				if(laserRanges[i] > 1.0f)
				{
					obj->flags |= LaserObjectState::flag_Extended;
				}
				// Threshold for laser segments to be considered instant
				MapTime laserSlamThreshold = (MapTime)ceil(state->tpStart->beatDuration / 8.0);
				if(obj->duration <= laserSlamThreshold && (obj->points[1] != obj->points[0]))
					obj->flags |= LaserObjectState::flag_Instant;

				// Link segments together
				if(state->last)
				{
					// Always fixup duration so they are connected by duration as well
					obj->prev = state->last;
					MapTime actualPrevDuration = obj->time - obj->prev->time;
					if(obj->prev->duration != actualPrevDuration)
					{
						obj->prev->duration = actualPrevDuration;
					}
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

					// Reset range extension
					laserRanges[i] = 1.0f;
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
				float pos = kshootMap.TranslateLaserChar(c) * laserRanges[i];
				if(laserRanges[i] > 1.0f)
				{
					if(c == 'C') // Snap edges to 0 or 1
					{
						pos = 0.0f;
					}
					else if(c == 'b')
					{
						pos = 1.0f;
					}
					else
					{
						pos -= (laserRanges[i] - 1.0f) * 0.5f;
					}
				}
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
	ObjectState::SortArray(m_objectStates);

	return true;
}