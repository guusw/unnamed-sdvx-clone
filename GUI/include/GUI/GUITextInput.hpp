#pragma once

struct GUITextInput
{
	// Apply text input to a string
	WString Apply(const WString& in) const;
	bool HasChanges() const;

	// Input string that was entered since last frame
	WString input;
	// Number of backspaces last frame
	uint32 backspaceCount = 0;
	// IME Composition string
	WString composition;
};