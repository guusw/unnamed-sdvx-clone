#include "stdafx.h"
#include "AudioEffects.hpp"

EffectDuration::EffectDuration(int32 duration /*= 0*/)
{
	this->duration = duration;
	type = Time;
}
EffectDuration::EffectDuration(float rate)
{
	this->rate = rate;
	type = Rate;
}

EffectDuration EffectDuration::Lerp(const EffectDuration& lhs, const EffectDuration& rhs, float time)
{
	assert(rhs.type == lhs.type);
	if(lhs.type == Type::Rate)
		return lhs.rate + (lhs.rate - rhs.rate) * time;
	else
		return (int32)(lhs.duration + (float)(lhs.duration - rhs.duration) * time);
}
uint32 EffectDuration::Absolute(double noteDuration)
{
	if(type == Time)
		return duration;
	else
		return (uint32)(rate * noteDuration);
}

static AudioEffect CreateDefault(EffectType type)
{
	AudioEffect ret;
	ret.type = type;

	typedef EffectParam<float> FloatRange;
	typedef EffectParam<int32> IntRange;
	typedef EffectParam<EffectDuration> TimeRange;

	// Default timing is 1/4
	ret.duration = TimeRange(0.25f);

	// Set defaults based on effect type
	switch(type)
	{
		// These are assumed to mostly be laser effect types (at least when used with the defaults)
	case EffectType::PeakingFilter:
	{
		Interpolation::Predefined easing = Interpolation::EaseInCubic;
		ret.peaking.freq = FloatRange(100.0f, 7000.0f, easing);
		ret.peaking.q = FloatRange(0.1f, 0.5f, easing);
		ret.peaking.gain = FloatRange(20.0f, 28.0f, easing);
		break;
	}
	case EffectType::LowPassFilter:
	{
		Interpolation::Predefined easing = Interpolation::EaseOutCubic;
		ret.lpf.freq = FloatRange(8000.0f, 200.0f, easing);
		ret.lpf.q = FloatRange(4.0f, 3.0f, easing);
		ret.lpf.gain = FloatRange(5.0f, 10.0f, easing);
		ret.lpf.peakQ = FloatRange(1.0f, 0.5f, easing);
		break;
	}
	case EffectType::HighPassFilter:
	{
		Interpolation::Predefined easing = Interpolation::EaseInCubic;
		ret.hpf.freq = FloatRange(100.0f, 2000.0f, easing);
		ret.hpf.q = FloatRange(1.0f, 1.0f);
		ret.hpf.gain = FloatRange(10.0f, 5.0f, easing);
		ret.hpf.peakQ = FloatRange(1.0f, 0.5f, easing);
		break;
	}
	case EffectType::Bitcrush:
		ret.bitcrusher.reduction = IntRange(0, 45);
		break;
	case EffectType::Gate:
		ret.gate.gate = 0.5f;
		break;
	case EffectType::Retrigger:
		ret.retrigger.gate = 0.7f;
		ret.retrigger.reset = TimeRange(0);
		break;
	case EffectType::Echo:
		ret.echo.feedback = FloatRange(0.25f);
		break;
	case EffectType::Panning:
		ret.panning.panning = FloatRange(0.0f);
		break;
	case EffectType::TapeStop:
		break;
	case EffectType::Phaser:
		ret.phaser.min = FloatRange(400.0f);
		ret.phaser.max = FloatRange(8000.0f);
		ret.phaser.feedback = FloatRange(0.3f);
		break;
	case EffectType::Wobble:
		// wobble is 1/12 by default
		ret.duration = TimeRange(1.0f / 12.0f);
		ret.wobble.startingFrequency = FloatRange(1500.0f);
		ret.wobble.frequency = FloatRange(500.0f);
		ret.wobble.q = FloatRange(2.0f);
		break;
	case EffectType::Flanger:
		ret.flanger.offset = IntRange(10);
		ret.flanger.depth = IntRange(40);
		break;
	}

	return ret;
}
class DefaultEffectSettings
{
public:
	Vector<AudioEffect> effects;
	DefaultEffectSettings()
	{
		// Preload all default effect settings
		effects.resize((size_t)EffectType::_Length);
		for(auto e : Enum_EffectType::GetMap())
		{
			effects[(size_t)e.first] = CreateDefault(e.first);
		}
	}
};

const AudioEffect& AudioEffect::GetDefault(EffectType type)
{
	static DefaultEffectSettings defaults;
	assert((size_t)type < defaults.effects.size());
	return defaults.effects[(size_t)type];
}
