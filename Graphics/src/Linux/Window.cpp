#include "stdafx.h"
#include "Window.hpp"
#include <X11/XKBlib.h>

namespace Graphics
{
	class Window_Impl
	{
	public:
		// Handle to outer class to send delegates
		Window& outer;
	public:
		Window_Impl(Window& outer, Vector2i size, const CustomWindowStyle& customStyle) : outer(outer)
		{
			m_caption = L"Window";

			// Open display
			m_display = XOpenDisplay(nullptr);
			assert(m_display);
			
			m_screen = DefaultScreen(m_display);
			
			// Create the window
			m_size = size;
			m_window = XCreateSimpleWindow(m_display, DefaultRootWindow(m_display), 
				0, 0, size.x, size.y, 5, 
				WhitePixel(m_display, m_screen), BlackPixel(m_display, m_screen));
			
			// Delete window message
			m_delWindow = XInternAtom(m_display, "WM_DELETE_WINDOW", 0 );
			XSetWMProtocols(m_display, m_window, &m_delWindow, 1);
			
			// Disable auto key-repeat messages
			XkbSetDetectableAutoRepeat(m_display, true, nullptr);
			
			// Handle input events
			m_evMask = KeyPressMask | KeyReleaseMask | ExposureMask;
			XSelectInput(m_display, m_window, m_evMask);
			
			// Show window by default
			Show();
			
		}
		~Window_Impl()
		{
		}

		void SetWindowSize(const Vector2i& size)
		{
		}
		Vector2i GetWindowSize()
		{
			return m_size;
		}
		void SetCaption(const WString& cap)
		{
		}
		bool Update()
		{
			XEvent ev;
			while(XPending(m_display) > 0)
			{
				XNextEvent(m_display, &ev);
				
				/* draw or redraw the window */
				if(ev.type==Expose) 
				{
					XFillRectangle(m_display, m_window, DefaultGC(m_display, m_screen), 20, 20, 10, 10);
				}
				
				/* exit on key press */
				if(ev.type==KeyPress)
				{
					Logf("Key press [%d]", Logger::Info, ev.xkey.keycode);
				}
				else if(ev.type==KeyRelease)
				{
					Logf("Key release [%d]", Logger::Info, ev.xkey.keycode);
				}

				// Handle Windows Close Event
				if(ev.type == ClientMessage)
				{
					if(ev.xclient.data.l[0] == m_delWindow)
						return false;
				}
			}
			return true;
		}
		void Hide()
		{
			XUnmapWindow(m_display, m_window);
		}
		void Show()
		{
			XMapWindow(m_display, m_window);
		}

		Atom m_delWindow;
		
		bool m_closed = false;
		Vector2i m_size;
		::Window m_window;
		Display* m_display;
		int m_screen;
		int m_evMask;
		WString m_caption;
	};

	Window::Window(Vector2i size, const CustomWindowStyle& customStyle)
	{
		m_impl = new Window_Impl(*this, size, customStyle);
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
		return (void*)m_impl->m_window;
	}
	void Window::SetCaption(const WString& cap)
	{
		m_impl->SetCaption(cap);
	}
	void Window::Close()
	{
		m_impl->m_closed = true;
	}
	void Window::SetWindowStyle(WindowStyle style)
	{
	}
	Vector2i Window::GetWindowSize()
	{
		return m_impl->GetWindowSize();
	}
	void Window::SetWindowSize(const Vector2i& size)
	{
		m_impl->SetWindowSize(size);
	}
	void Window::SwitchFullscreen(uint32 monitorID)
	{
	}
	void Window::SetStyles(uint32 mask)
	{
	}
	void Window::UnsetStyles(uint32 mask)
	{
	}
	uint32 Window::HasStyle(uint32 mask)
	{
		return 0;
	}
}