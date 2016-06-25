#pragma once
#include <Graphics/CustomWindowStyle.hpp>
#include <Graphics/Keys.hpp>

namespace Graphics
{
	// Windowed or bordered window style
	enum class WindowStyle
	{
		Windowed, Borderless
	};

	/*
		Simple window class that manages window messages, window style and input
		Renamed from Window to DesktopWindow to avoid conflicts with libX11 on Linux
	*/
	class Window : Unique
	{
	public:
		Window(Vector2i size = Vector2i(800, 600), const CustomWindowStyle& customStyle = CustomWindowStyle::Default);
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

		// Window Client area size
		Vector2i GetWindowSize();
		// Set window client area size
		void SetWindowSize(const Vector2i& size);
		void SwitchFullscreen(uint32 monitorID = -1);

		// WINDOWS ONLY
		/// TODO: Replace mask with platform-agnostic defines
		// Sets certain bits of the window style
		void SetStyles(uint32 mask);
		// Unsets styles from the window style
		void UnsetStyles(uint32 mask);
		// Checks the given bits in the window style and returns the AND'ed result
		uint32 HasStyle(uint32 mask);
		
		// Checks if a key is pressed
		void IsKeyPressed(Key key) const;

		Delegate<Key> OnKeyPressed;
		Delegate<Key> OnKeyReleased;
#ifdef _WIN32
		// Delegate called for window messages
		Delegate<HWND, UINT, WPARAM, LPARAM> OnWindowMessage;
#endif
		Delegate<const Vector2i&> OnResized;

	private:
		class Window_Impl* m_impl;
	};
}