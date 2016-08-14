#pragma once

enum class TimeFunction
{
	Linear,
	EaseOutQuad,
	EaseInQuad,
};

// GUI Animation object
class IGUIAnimation : public RefCounted<IGUIAnimation>
{
public:
	virtual ~IGUIAnimation() = default;
	// Return false when done
	virtual bool Update(float deltaTime) = 0;
	// Target value of the animation
	virtual void* GetTarget() = 0;

	// Timing function used
	TimeFunction timeFunction = TimeFunction::Linear;

	template<typename T, typename Lambda>
	static Ref<IGUIAnimation> CreateCallback(T next, T last, float duration, Lambda&& l, uint32 identifier);
	template<typename T>
	static Ref<IGUIAnimation> Create(T* target, T next, float duration);
	template<typename T>
	static Ref<IGUIAnimation> Create(T* target, T next, T last, float duration);

protected:
	template<typename T>
	T Lerp(T a, T b, float f)
	{
		return a + (b - a) * f;
	}
	int32 Lerp(int32 a, int32 b, float f)
	{
		return a + (int32)((float)(b - a) * f);
	}
	float Timing(float t)
	{
		switch(timeFunction)
		{
		case TimeFunction::Linear:
			return t;
		case TimeFunction::EaseInQuad:
			return t*t;
		case TimeFunction::EaseOutQuad:
			return t*(2 - t);
		}
		assert(false); // Invalid timing function
		return t;
	}
};

// A templated animation of a single vector/float/color variable
template<typename T>
class GUIAnimation : public IGUIAnimation
{
public:
	// A->B animation with A set to the current value
	GUIAnimation(T* target, T newValue, float duration)
	{
		assert(target);
		m_target = target;
		m_duration = duration;
		m_last = m_target[0];
		m_next = newValue;
	}
	// A->B animation with A and B provided
	GUIAnimation(T* target, T newValue, T lastValue, float duration)
	{
		assert(target);
		m_target = target;
		m_duration = duration;
		m_last = lastValue;
		m_next = newValue;
	}
	virtual bool Update(float deltaTime) override
	{
		if(m_time >= m_duration)
			return false;

		m_time += deltaTime;
		float r = m_time / m_duration;
		if(m_time >= m_duration)
		{
			r = 1.0f;
			m_time = m_duration;
		}

		T current = Lerp(m_last, m_next, Timing(r));
		m_target[0] = current;

		return r < 1.0f;
	}
	// Target of the animation
	virtual void* GetTarget() override
	{
		return m_target;
	}
private:
	T m_last;
	T m_next;
	T* m_target;
	float m_time = 0.0f;
	float m_duration;
};

// A templated animation of a single vector/float/color variable
template<typename T, typename Lambda>
class GUICallbackAnimation : public IGUIAnimation
{
public:
	// A->B animation with A and B provided, calls l on every update
	GUICallbackAnimation(Lambda&& l, T newValue, T lastValue, float duration, uint32 identifier) : m_lambda(l)
	{
		m_identifier = identifier;
		m_duration = duration;
		m_last = lastValue;
		m_next = newValue;
	}
	virtual bool Update(float deltaTime) override
	{
		if(m_time >= m_duration)
			return false;

		m_time += deltaTime;
		float r = m_time / m_duration;
		if(m_time >= m_duration)
		{
			r = 1.0f;
			m_time = m_duration;
		}

		T current = Lerp(m_last, m_next, r);
		m_lambda(current);

		return r < 1.0f;
	}
	// Target of the animation
	virtual void* GetTarget() override
	{
		return (void*)m_identifier;
	}
private:
	size_t m_identifier;
	Lambda m_lambda;
	T m_last;
	T m_next;
	float m_time = 0.0f;
	float m_duration;
};

template<typename T, typename Lambda>
Ref<IGUIAnimation> IGUIAnimation::CreateCallback(T next, T last, float duration, Lambda&& l, uint32 identifier)
{
	return Ref<IGUIAnimation>(new GUICallbackAnimation<T, Lambda>(std::forward<Lambda>(l), next, last, duration, identifier));
}
template<typename T>
Ref<IGUIAnimation> IGUIAnimation::Create(T* target, T next, float duration)
{
	return Ref<IGUIAnimation>(new GUIAnimation<T>(target, next, duration));
}
template<typename T>
Ref<IGUIAnimation> IGUIAnimation::Create(T* target, T next, T last, float duration)
{
	return Ref<IGUIAnimation>(new GUIAnimation<T>(target, next, last, duration));
}