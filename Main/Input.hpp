#pragma once

/*
	Class that handles game keyboard (and soon controller input)
*/
class Input : Unique
{
public:
	enum class Button
	{
		BT_0,
		BT_1,
		BT_2,
		BT_3,
		FX_0,
		FX_1,
		LS_0Neg, // Left laser- 
		LS_0Pos, // Left laser+		(|---->)
		LS_1Neg, // Right laser-	(<----|)
		LS_1Pos, // Right laser+
		Length,
	};

    void Init(DesktopWindow& wnd);
	void Cleanup();

	bool GetButton(Button button) const;

	// Event handlers
	virtual void OnKeyPressed(Key key);
	virtual void OnKeyReleased(Key key);

	// Request laser input state
	float GetInputLaserDir(uint32 laserIdx);

	// Button delegates
	Delegate<Button> OnButtonPressed;
	Delegate<Button> OnButtonReleased;

private:
	void m_InitButtonMapping();
	void m_OnButtonInput(Button b, bool pressed);

	Map<Key, Button> m_buttonMap;
	bool m_buttonStates[(size_t)Button::Length];
	float m_laserStates[2] = { 0.0f };

    DesktopWindow* m_window = nullptr;
};