#pragma once
#include <Graphics/Keys.hpp>

namespace Graphics
{
	class KeyMap
	{
	public:
		KeyMap()
		{
		}
		void AddMapping(uint32 keyCode, Key key)
		{
			m_keyMapping.Add(keyCode, key);
		}
		// Add a range from keyCodeStart to keyCodeEnd(inclusive)
		void AddRangeMapping(uint32 keyCodeStart, uint32 keyCodeEnd, Key startKey)
		{
			uint8 k = (uint8)startKey;
			for(uint32 i = keyCodeStart; i < keyCodeEnd; i++)
			{
				AddMapping(i, (Key)k);
				k++;
			}
		}
		Key Translate(uint32 keyCode) const
		{
			const Key* k = m_keyMapping.Find(keyCode);
			if(k)
				return *k;
			return Key::None;
		}
	private:
		Map<uint32, Key> m_keyMapping;
	};
}