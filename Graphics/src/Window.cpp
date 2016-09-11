#include "stdafx.h"
#include "Window.hpp"
#include "KeyMap.hpp"
#include "Image.hpp"
#include "Gamepad_Impl.hpp"


namespace Graphics
{
	/* SDL Instance singleton */
	class SDL
	{
	protected:
		SDL()
		{
			SDL_SetMainReady();
			int r = SDL_Init(SDL_INIT_VIDEO | SDL_INIT_GAMECONTROLLER);
			assert(r == 0);
		}
	public:
		~SDL()
		{
			SDL_Quit();
		}
		static SDL& Main()
		{
			static SDL sdl;
			return sdl;
		}
	};

	class Window_Impl
	{
	public:
		// Handle to outer class to send delegates
		Window& outer;
	public:
		Window_Impl(Window& outer, Vector2i size) : outer(outer)
		{
			SDL::Main();

			// Initialize button mapping
			m_keyMapping.AddRangeMapping('a', 'z', Key::A);
			m_keyMapping.AddRangeMapping('0', '9', Key::Top0);
			m_keyMapping.AddRangeMapping(SDLK_F1, SDLK_F12, Key::F1);
			m_keyMapping.AddMapping(SDLK_PRINTSCREEN, Key::PrntScr);
			m_keyMapping.AddMapping(SDLK_SCROLLLOCK, Key::ScrollLock);
			m_keyMapping.AddMapping(SDLK_PAUSE, Key::Pause);
			m_keyMapping.AddMapping(SDLK_ESCAPE, Key::Escape);
			m_keyMapping.AddMapping(SDLK_BACKQUOTE, Key::Tilde);
			m_keyMapping.AddMapping(SDLK_PAGEUP, Key::PageUp);
			m_keyMapping.AddMapping(SDLK_PAGEDOWN, Key::PageDown);
			m_keyMapping.AddMapping(SDLK_RETURN, Key::Return);
			m_keyMapping.AddMapping(SDLK_PLUS, Key::Plus);
			m_keyMapping.AddMapping(SDLK_MINUS, Key::Minus);
			m_keyMapping.AddMapping(SDLK_LEFT, Key::ArrowLeft);
			m_keyMapping.AddMapping(SDLK_RIGHT, Key::ArrowRight);
			m_keyMapping.AddMapping(SDLK_UP, Key::ArrowUp);
			m_keyMapping.AddMapping(SDLK_DOWN, Key::ArrowDown);
			m_keyMapping.AddMapping(SDLK_SPACE, Key::Space);
			m_keyMapping.AddMapping(SDLK_BACKSPACE, Key::Backspace);
			m_keyMapping.AddMapping(SDLK_TAB, Key::Tab);

			m_clntSize = size;

			m_caption = L"Window";
			String titleUtf8 = Utility::ConvertToUTF8(m_caption);
			m_window = SDL_CreateWindow(*titleUtf8, SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
				m_clntSize.x, m_clntSize.y, SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE);
			assert(m_window);

			uint32 numJoysticks = SDL_NumJoysticks();
			if(numJoysticks == 0)
			{
				Logf("No joysticks found", Logger::Warning);
			}
			else
			{
				Logf("Listing %d Joysticks:", Logger::Info, numJoysticks);
				for(uint32 i = 0; i < numJoysticks; i++)
				{
					SDL_Joystick* joystick = SDL_JoystickOpen(i);
					if(!joystick)
					{
						Logf("[%d] <failed to open>", Logger::Warning, i);
						continue;
					}
					String deviceName = SDL_JoystickName(joystick);

					Logf("[%d] \"%s\" (%d buttons, %d axes, %d hats)", Logger::Info,
						i, deviceName, SDL_JoystickNumButtons(joystick), SDL_JoystickNumAxes(joystick), SDL_JoystickNumHats(joystick));

					SDL_JoystickClose(joystick);
				}
			}

		}
		~Window_Impl()
		{
			// Release gamepads
			for(auto it : m_gamepads)
			{
				it.second.Destroy();
			}

			SDL_DestroyWindow(m_window);
		}

		void SetWindowPos(const Vector2i& pos)
		{
			SDL_SetWindowPosition(m_window, pos.x, pos.y);
		}
		Vector2i GetWindowPos() const
		{
			Vector2i res;
			SDL_GetWindowPosition(m_window, &res.x, &res.y);
			return res;
		}

		void SetWindowSize(const Vector2i& size)
		{
			SDL_SetWindowSize(m_window, size.x, size.y);
		}
		Vector2i GetWindowSize() const
		{
			Vector2i res;
			SDL_GetWindowSize(m_window, &res.x, &res.y);
			return res;
		}

		void SetWindowStyle(WindowStyle style)
		{
		}

		/* input handling */
		void HandleKeyEvent(SDL_Keycode code, uint8 newState, int32 repeat)
		{
			SDL_Keymod m = SDL_GetModState();
			m_modKeys = ModifierKeys::None;
			if((m & KMOD_ALT) != 0)
			{
				(uint8&)m_modKeys |= (uint8)ModifierKeys::Alt;
			}
			if((m & KMOD_CTRL) != 0)
			{
				(uint8&)m_modKeys |= (uint8)ModifierKeys::Ctrl;
			}
			if((m & KMOD_SHIFT) != 0)
			{
				(uint8&)m_modKeys |= (uint8)ModifierKeys::Shift;
			}

			Graphics::Key keyCode = m_keyMapping.Translate(code);
			if(keyCode == Key::None)
				return; // Not mapped

			uint8& currentState = m_keyStates[(uint8)keyCode];
			if(currentState != newState)
			{
				currentState = newState;
				if(newState == 1)
				{
					outer.OnKeyPressed.Call(keyCode);
				}
				else
				{
					outer.OnKeyReleased.Call(keyCode);
				}
			}
			if(currentState == 1)
			{
				outer.OnKeyRepeat.Call(keyCode);
			}
		}

		/* Window show hide, positioning, etc.*/
		void Show()
		{
			SDL_ShowWindow(m_window);
		}
		void Hide()
		{
			SDL_HideWindow(m_window);
		}
		void SetCaption(const WString& cap)
		{
			m_caption = L"Window";
			String titleUtf8 = Utility::ConvertToUTF8(m_caption);
			SDL_SetWindowTitle(m_window, *titleUtf8);
		}

		void SetCursor(Ref<class ImageRes> image, Vector2i hotspot)
		{
#ifdef _WIN32
			if(currentCursor)
			{
				SDL_FreeCursor(currentCursor);
				currentCursor = nullptr;
			}
			if(image)
			{
				Vector2i size = image->GetSize();
				void* bits = image->GetBits();
				SDL_Surface* surf = SDL_CreateRGBSurfaceFrom(bits, size.x, size.y, 32, size.x * 4,
					0x000000FF, 0x0000FF00, 0x00FF0000, 0xFF000000);
				if(surf)
				{
					currentCursor = SDL_CreateColorCursor(surf, hotspot.x, hotspot.y);
				}
			}
			SDL_SetCursor(currentCursor);
#endif
/// NOTE: Cursor transparency is broken on linux
		}

		// Update loop
		Timer t;
		bool Update()
		{
			SDL_Event evt;
			while(SDL_PollEvent(&evt))
			{
				if(evt.type == SDL_EventType::SDL_KEYDOWN)
				{
					if(m_textComposition.composition.empty())
					{
						// Ignore key input when composition is being typed
						HandleKeyEvent(evt.key.keysym.sym, 1, evt.key.repeat);
					}
				}
				else if(evt.type == SDL_EventType::SDL_KEYUP)
				{
					HandleKeyEvent(evt.key.keysym.sym, 0, 0);
				}
				else if(evt.type == SDL_EventType::SDL_JOYBUTTONDOWN)
				{
					m_joystickMap[evt.jbutton.which]->HandleInputEvent(evt.jbutton.button, true);
				}
				else if(evt.type == SDL_EventType::SDL_JOYBUTTONUP)
				{
					m_joystickMap[evt.jbutton.which]->HandleInputEvent(evt.jbutton.button, false);
				}
				else if(evt.type == SDL_EventType::SDL_JOYAXISMOTION)
				{
					m_joystickMap[evt.jbutton.which]->HandleAxisEvent(evt.jaxis.axis, evt.jaxis.value);
				}
				else if(evt.type == SDL_EventType::SDL_JOYHATMOTION)
				{
					m_joystickMap[evt.jbutton.which]->HandleHatEvent(evt.jhat.hat, evt.jhat.value);
				}
				else if(evt.type == SDL_EventType::SDL_MOUSEBUTTONDOWN)
				{
					switch(evt.button.button)
					{
					case SDL_BUTTON_LEFT:
						outer.OnMousePressed.Call(MouseButton::Left);
							break;
					case SDL_BUTTON_MIDDLE:
						outer.OnMousePressed.Call(MouseButton::Middle);
							break;
					case SDL_BUTTON_RIGHT:
						outer.OnMousePressed.Call(MouseButton::Right);
							break;
					}
				}
				else if(evt.type == SDL_EventType::SDL_MOUSEBUTTONUP)
				{
					switch(evt.button.button)
					{
					case SDL_BUTTON_LEFT:
						outer.OnMouseReleased.Call(MouseButton::Left);
						break;
					case SDL_BUTTON_MIDDLE:
						outer.OnMouseReleased.Call(MouseButton::Middle);
						break;
					case SDL_BUTTON_RIGHT:
						outer.OnMouseReleased.Call(MouseButton::Right);
						break;
					}
				}
				else if(evt.type == SDL_EventType::SDL_MOUSEWHEEL)
				{
					outer.OnMouseScroll.Call(evt.wheel.y);
				}
				else if(evt.type == SDL_EventType::SDL_QUIT)
				{
					m_closed = true;
				}
				else if(evt.type == SDL_EventType::SDL_WINDOWEVENT)
				{
					if(evt.window.windowID == SDL_GetWindowID(m_window))
					{
						if(evt.window.event == SDL_WindowEventID::SDL_WINDOWEVENT_RESIZED)
						{
							Vector2i newSize(evt.window.data1, evt.window.data2);
							outer.OnResized.Call(newSize);
						}
					}
				}
				else if(evt.type == SDL_EventType::SDL_TEXTINPUT)
				{
					WString wstr = Utility::ConvertToWString(evt.text.text);
					outer.OnTextInput.Call(wstr);
				}
				else if(evt.type == SDL_EventType::SDL_TEXTEDITING)
				{
					SDL_Rect scr;
					SDL_GetWindowPosition(m_window, &scr.x, &scr.y);
					SDL_GetWindowSize(m_window, &scr.w, &scr.h);
					SDL_SetTextInputRect(&scr);

					m_textComposition.composition = Utility::ConvertToWString(evt.edit.text);
					m_textComposition.cursor = evt.edit.start;
					m_textComposition.selectionLength = evt.edit.length;
					outer.OnTextComposition.Call(m_textComposition);
				}
			}
			return !m_closed;
		}

		void SwitchFullscreen(uint32 monitorID)
		{
			if(m_fullscreen)
			{
				SDL_SetWindowFullscreen(m_window, 0);
				m_fullscreen = false;
			}
			else
			{
				SDL_SetWindowFullscreen(m_window, SDL_WINDOW_FULLSCREEN);
				m_fullscreen = true;
			}
		}
		bool IsFullscreen() const
		{
			return m_fullscreen;
		}

		SDL_Window* m_window;

		SDL_Cursor* currentCursor = nullptr;

		// Window Input State
		uint8 m_keyStates[256] = { 0 };
		KeyMap m_keyMapping;
		ModifierKeys m_modKeys = ModifierKeys::None;

		// Gamepad input
		Map<int32, Ref<Gamepad_Impl>> m_gamepads;
		Map<SDL_JoystickID, Gamepad_Impl*> m_joystickMap;

		// Text input / IME stuff
		TextComposition m_textComposition;

		// Various window state
		bool m_active = true;
		bool m_closed = false;
		bool m_fullscreen = false;
		uint32 m_style;
		Vector2i m_clntSize;
		WString m_caption;
	};

	Window::Window(Vector2i size)
	{
		m_impl = new Window_Impl(*this, size);
	}
	Window::~Window()
	{
		delete m_impl;
	}
	void Window::Show()
	{
		m_impl->Show();
	}
	void Window::Hide()
	{
		m_impl->Hide();
	}
	bool Window::Update()
	{
		return m_impl->Update();
	}
	void* Window::Handle()
	{
		return m_impl->m_window;
	}
	void Window::SetCaption(const WString& cap)
	{
		m_impl->SetCaption(cap);
	}
	void Window::Close()
	{
		m_impl->m_closed = true;
	}

	Vector2i Window::GetMousePos()
	{
		Vector2i res;
		SDL_GetMouseState(&res.x, &res.y);
		return res;
	}
	void Window::SetCursor(Ref<class ImageRes> image, Vector2i hotspot /*= Vector2i(0,0)*/)
	{
		m_impl->SetCursor(image, hotspot);
	}
	void Window::SetCursorVisible(bool visible)
	{
		SDL_ShowCursor(visible);
	}

	void Window::SetWindowStyle(WindowStyle style)
	{
		m_impl->SetWindowStyle(style);
	}
	Vector2i Window::GetWindowPos() const
	{
		return m_impl->GetWindowPos();
	}
	void Window::SetWindowPos(const Vector2i& pos)
	{
		m_impl->SetWindowPos(pos);
	}

	Vector2i Window::GetWindowSize() const
{
		return m_impl->GetWindowSize();
	}
	void Window::SetWindowSize(const Vector2i& size)
	{
		m_impl->SetWindowSize(size);
	}
	void Window::SwitchFullscreen(uint32 monitorID)
	{
		m_impl->SwitchFullscreen(monitorID);
	}
	bool Window::IsFullscreen() const
	{
		return m_impl->IsFullscreen();
	}

	bool Window::IsKeyPressed(Key key) const
	{
		return m_impl->m_keyStates[(uint8)key] > 0;
	}

	Graphics::ModifierKeys Window::GetModifierKeys() const
	{
		return m_impl->m_modKeys;
	}

	void Window::StartTextInput()
	{
		SDL_StartTextInput();
	}
	void Window::StopTextInput()
	{
		SDL_StopTextInput();
	}
	const Graphics::TextComposition& Window::GetTextComposition() const
	{
		return m_impl->m_textComposition;
	}

	WString Window::GetClipboard() const
	{
		char* utf8Clipboard = SDL_GetClipboardText();
		WString ret = Utility::ConvertToWString(utf8Clipboard);
		SDL_free(utf8Clipboard);

		return ret;
	}

	int32 Window::GetNumGamepads() const
	{
		return SDL_NumJoysticks();
	}
	Ref<Gamepad> Window::OpenGamepad(int32 deviceIndex)
	{
		Ref<Gamepad_Impl>* openGamepad = m_impl->m_gamepads.Find(deviceIndex);
		if(openGamepad)
			return openGamepad->As<Gamepad>();
		Ref<Gamepad_Impl> newGamepad;

		Gamepad_Impl* gamepadImpl = new Gamepad_Impl();
		// Try to initialize new device
		if(gamepadImpl->Init(this, deviceIndex))
		{
			newGamepad = Ref<Gamepad_Impl>(gamepadImpl);

			// Receive joystick events
			SDL_JoystickEventState(SDL_ENABLE);
		}
		else
		{
			delete gamepadImpl;
		}
		if(newGamepad)
		{
			m_impl->m_gamepads.Add(deviceIndex, newGamepad);
			m_impl->m_joystickMap.Add(SDL_JoystickInstanceID(gamepadImpl->m_joystick), gamepadImpl);
		}
		return newGamepad.As<Gamepad>();
	}

	void Window::SetMousePos(const Vector2i& pos)
	{
		SDL_WarpMouseInWindow(m_impl->m_window, pos.x, pos.y);
	}

}

namespace Graphics
{
	ImplementBitflagEnum(ModifierKeys);
}
