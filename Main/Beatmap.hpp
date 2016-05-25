#pragma once
#include "BeatmapObjects.hpp"

/* Global settings stored in a beatmap */
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
	MapTime offset;
	// Both audio tracks specified for the map / if any is set
	String audioNoFX;
	String audioFX;
	// Path to the jacket image
	String jacketPath;

	// Initial audio settings
	float slamVolume = 1.0f;
	float laserEffectMix = 1.0f;
	LaserEffectType laserEffectType = LaserEffectType::PeakingFilter;

};

/*
	Generic beatmap format, Can either load it's own format or KShoot maps
*/
class Beatmap
{
public:
	virtual ~Beatmap();
	bool Load(BinaryStream& input);
	// Saves the map as it's own format
	bool Save(BinaryStream& output);

	// Returns the settings of the map, contains metadata + song/image paths.
	const BeatmapSettings& GetMapSettings();

	// Vector of timing points in the map, sorted by when they appear in the map
	// Must keep the beatmap class instance alive for these to stay valid
	// Can contain multiple objects at the same time
	const Vector<TimingPoint*>& GetLinearTimingPoints() const;
	// Vector of objects in the map, sorted by when they appear in the map
	// Must keep the beatmap class instance alive for these to stay valid
	// Can contain multiple objects at the same time
	const Vector<ObjectState*>& GetLinearObjects() const;
	// Vector of zoom control points in the map, sorted by when they appear in the map
	// Must keep the beatmap class instance alive for these to stay valid
	// Can contain multiple objects at the same time
	const Vector<ZoomControlPoint*>& GetZoomControlPoints() const;

private:
	bool m_ProcessKShootMap(BinaryStream& input);
	bool m_Serialize(BinaryStream& stream);

	Vector<TimingPoint*> m_timingPoints;
	Vector<ObjectState*> m_objectStates;
	Vector<ZoomControlPoint*> m_zoomControlPoints;
	BeatmapSettings m_settings;
};