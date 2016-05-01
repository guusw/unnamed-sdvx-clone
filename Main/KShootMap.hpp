#pragma once

using Utility::Sprintf;

/*
	Any division inside a KShootBlock
*/
class KShootTick
{
public:
	String ToString() const;
	void Clear();

	Map<String, String> settings;

	// Original data for this tick
	String buttons, fx, laser, add;
};
/* 
	A single bar in the map file 
*/
class KShootBlock
{
public:
	Vector<KShootTick> ticks;
};
class KShootTime
{
public:
	KShootTime();;
	KShootTime(uint32_t block, uint32_t tick);;
	operator bool() const;
	uint32_t block;
	uint32_t tick;
};

/* 
	Map class for that splits up maps in the ksh format into Ticks and Blocks
*/
class KShootMap
{
public:
	class TickIterator
	{
	public:
		TickIterator(KShootMap& map, KShootTime start = KShootTime(0, 0));
		TickIterator& operator++();
		operator bool() const;
		KShootTick& operator*();
		KShootTick* operator->();
		const KShootTime& GetTime() const;
		const KShootBlock& GetCurrentBlock() const;
	private:
		KShootMap& m_map;
		KShootBlock* m_currentBlock;
		KShootTime m_time;
	};

public:
	KShootMap();
	~KShootMap();
	bool Init(BinaryStream& input);
	bool GetBlock(const KShootTime& time, KShootBlock*& tickOut);
	bool GetTick(const KShootTime& time, KShootTick*& tickOut);
	float TimeToFloat(const KShootTime& time) const;
	float TranslateLaserChar(char c) const;

	Map<String, String> settings;
	Vector<KShootBlock> blocks;

private:
	static const uint32_t KShootMap::c_laserStart;
	static const uint32_t KShootMap::c_laserEnd;
	static const uint32_t KShootMap::c_laserRange;
	static const uint32_t c_magic;
	static const char* c_sep;

};