#pragma once

typedef uint64 MapTime;
struct ButtonState
{
	// Used for hold notes, -1 is a normal note
	MapTime duration = 0;
	bool on = false;
	uint8 effectType = 0;
};
struct LaserState
{
	LaserState() { }
	void Reset(int32 index);
	// Position of the laser on the track
	float points[2];
	// Duration of laser
	MapTime duration;
	// Set the to the object state that connects to this laser, if any, otherwise null
	struct ObjectState* next;
	struct ObjectState* prev;
};
struct ObjectState
{
	static bool StaticSerialize(BinaryStream& stream, ObjectState*& obj);
	bool IsEmpty();
	// Returns the maximum duration of this object
	// returns 0 if this object contains only instant notes
	MapTime CalculateMaxDuration() const;

	MapTime maxDuration;
	ButtonState buttons[6];
	LaserState lasers[2];
	MapTime time;
}; 

struct TimingPoint
{
	static bool StaticSerialize(BinaryStream& stream, TimingPoint*& obj);
	MapTime GetSlamTreshold() const;

	// beat duration in milliseconds
	double beatDuration;
	// Measure / Beat, mostly 4 or 3
	uint8 measure;
	// Timing offset in milliseconds
	MapTime offset;
};

struct BeatmapSettings
{
	// Basic song meta data
	WString title;
	WString artist;
	WString effector;
	WString illustrator;
	WString tags;
	// Reported BPM range by the map
	WString bpm;
	// Offset in ms for the map to start
	uint64 offset;
	// Both audio tracks specified for the map / if any is set
	String audioNoFX;
	String audioFX;
	// Path to the jacket image
	String jacketPath;
};

/*
	Generic beatmap format, Can either load it's own format or KShoot maps
*/
class Beatmap
{
public:
	bool Load(BinaryStream& input);
	// Saves the map as it's own format
	bool Save(BinaryStream& output);

	// Returns the settings of the map, contains metadata + song/image paths.
	const BeatmapSettings& GetMapSettings();

	// Vector of timing points in the map, sorted by when they appear in the map
	// Must keep the beatmap class instance alive for these to stay valid
	Vector<TimingPoint*> GetLinearTimingPoints();
	// Vector of objects in the map, sorted by when they appear in the map
	// Must keep the beatmap class instance alive for these to stay valid
	Vector<ObjectState*> GetLinearObjects();

private:
	void m_ProcessKShootMap(class KShootMap& kshootMap);
	bool m_Serialize(BinaryStream& stream);

	Map<MapTime, TimingPoint*> m_timingPoints;
	Map<MapTime, ObjectState*> m_objectStates;
	BeatmapSettings m_settings;
};