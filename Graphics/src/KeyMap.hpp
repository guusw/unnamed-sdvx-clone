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
		void AddMapping(uint16 keyCode, Key key)
		{
			m_keyMapping.Add(keyCode, key);
		}
		// Add a range from keyCodeStart to keyCodeEnd(inclusive)
		void AddRangeMapping(uint16 keyCodeStart, uint16 keyCodeEnd, Key startKey)
		{
			uint16 k = (uint16)startKey;
			for(uint16 i = keyCodeStart; i < keyCodeEnd; i++)
			{
				AddMapping(i, (Key)k);
				k++;
			}
		}
		Key Translate(uint16 keyCode) const
		{
			const Key* k = m_keyMapping.Find(keyCode);
			if(k)
				return *k;
			return Key::None;
		}
	private:
		Map<uint16, Key> m_keyMapping;
	};
}