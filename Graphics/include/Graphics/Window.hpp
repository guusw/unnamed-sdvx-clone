#pragma once
#include <Graphics/Keys.hpp>

namespace Graphics
{
	// Windowed or bordered window style
	enum class WindowStyle
	{
		Windowed, Borderless
	};

	// Text input data
	struct TextComposition
	{
		WString composition;
		int32 cursor;
		int32 selectionLength;
	};

	/*
		Simple window class that manages window messages, window style and input
		Renamed from Window to DesktopWindow to avoid conflicts with libX11 on Linux
	*/
	class Window : Unique
	{
	public:
		Window(Vector2i size = Vector2i(800, 600));
		~Window();
		// Show the window
		void Show();
		// Hide the window
		void Hide();
		// Call every frame to update the window message loop
		// returns false if the window received a close message
		bool Update();
		// On windows: returns the HWND
		void* Handle();
		// Set the window title (caption)
		void SetCaption(const WString& cap);
		// Closes the window
		void Close();

		// Switches between borderless and windowed
		void SetWindowStyle(WindowStyle style);

		// Get full window position
		Vector2i GetWindowPos() const;
		// Set full window position
		void SetWindowPos(const Vector2i& pos);

		// Window Client area size
		Vector2i GetWindowSize() const;
		// Set window client area size
		void SetWindowSize(const Vector2i& size);
		void SwitchFullscreen(uint32 monitorID = -1); 
		
		// Checks if a key is pressed
		bool IsKeyPressed(Key key) const;

		ModifierKeys GetModifierKeys() const;

		// Start allowing text input
		void StartTextInput();
		// Stop allowing text input
		void StopTextInput();
		// Used to get current IME working data
		const TextComposition& GetTextComposition() const;

		Delegate<Key> OnKeyPressed;
		Delegate<Key> OnKeyReleased;
		Delegate<const WString&> OnTextInput;
		Delegate<const TextComposition&> OnTextComposition;
		Delegate<const Vector2i&> OnResized;

	private:
		class Window_Impl* m_impl;
	};
}