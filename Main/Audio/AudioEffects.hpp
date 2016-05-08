#pragma once

// The types of effects that can be used on the effect buttons
enum class EffectType : uint8
{
	None = 0,
	Retrigger,
	Flanger,
	Phaser,
	Gate,
	TapeStop,
	Bitcrush,
	Wobble,
	SideChain,
};

// The types of effects that can be used on lasers
enum class LaserEffectType : uint8
{
	None = 0,
	LowPassFilter,
	HighPassFilter,
	PeakingFilter,
	Bitcrush,
};

typedef uint8 EffectParam;
