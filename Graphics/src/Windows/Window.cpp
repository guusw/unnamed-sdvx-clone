#include "stdafx.h"
#include "Window.hpp"
#include "HBitmap.hpp"
#include "KeyMap.hpp"
#include <Windowsx.h>
#include <Uxtheme.h>
#include <dwmapi.h>
#pragma comment(lib, "dwmapi.lib")
#pragma comment(lib, "uxtheme.lib")

namespace Graphics
{
#define RECTWIDTH(__r) ((__r).right-(__r).left)
#define RECTHEIGHT(__r) ((__r).bottom-(__r).top)
#define DCX_USESTYLE 0x00010000

	static LRESULT CALLBACK WndProc(HWND wnd, UINT msg, WPARAM wp, LPARAM lp);
	static HINSTANCE hinst = GetModuleHandle(nullptr);
	static int wndIdx = 0;

	class Window_Impl
	{
	public:
		// Handle to outer class to send delegates
		Window& outer;
	public:
		Window_Impl(Window& outer, Vector2i size, const CustomWindowStyle& customStyle) : outer(outer)
		{
			// Initialize button mapping
			m_keyMapping.AddRangeMapping('A', 'Z', Key::A);
			m_keyMapping.AddRangeMapping('0', '9', Key::Top0);
			m_keyMapping.AddRangeMapping(VK_F1, VK_F12, Key::F1);
			m_keyMapping.AddMapping(VK_PRINT, Key::PrntScr);
			m_keyMapping.AddMapping(VK_SCROLL, Key::ScrollLock);
			m_keyMapping.AddMapping(VK_PAUSE, Key::Pause);
			m_keyMapping.AddMapping(VK_ESCAPE, Key::Escape);
			m_keyMapping.AddMapping(VK_OEM_3, Key::Tilde);
			m_keyMapping.AddMapping(VK_PRIOR, Key::PageUp);
			m_keyMapping.AddMapping(VK_NEXT, Key::PageDown);
			m_keyMapping.AddMapping(VK_RETURN, Key::Return);
			m_keyMapping.AddMapping(VK_ADD, Key::Plus);
			m_keyMapping.AddMapping(VK_SUBTRACT, Key::Minus);
			m_keyMapping.AddMapping(VK_LEFT, Key::ArrowLeft);
			m_keyMapping.AddMapping(VK_RIGHT, Key::ArrowRight);
			m_keyMapping.AddMapping(VK_UP, Key::ArrowUp);
			m_keyMapping.AddMapping(VK_DOWN, Key::ArrowDown);
			m_keyMapping.AddMapping(VK_SPACE, Key::Space);

			m_customWindowStyle = customStyle;

			Utility::MemsetZero(m_wndClass);
			m_wndClass.cbSize = sizeof(WNDCLASSEX);
			m_wndClass.lpfnWndProc = &WndProc;
			m_wndClass.hInstance = hinst;
			m_wndClass.hCursor = LoadCursor(nullptr, IDC_CROSS);
			m_wndClass.hbrBackground = (HBRUSH)GetStockObject(BLACK_BRUSH);

			m_caption = L"Window";

			// Create application lifetime unique window class name
			char moduleNameBuffer[MAX_PATH];
			GetModuleFileNameA(hinst, moduleNameBuffer, MAX_PATH);
			String moduleName;
			Path::RemoveLast(moduleNameBuffer, &moduleName);

			m_className = Utility::ConvertToUnicode(Utility::Sprintf("%s_fx_%d", moduleName, wndIdx++));
			m_wndClass.lpszClassName = *m_className;

			HRESULT res = RegisterClassEx(&m_wndClass);
			assert(res);

			m_style = WS_OVERLAPPEDWINDOW | WS_VISIBLE;

			// Resize window rect to client size
			RECT wndRect = { 0, 0, size.x, size.y };
			AdjustWindowRect(wndRect);

			// Create the window and make sure it was successfull
			m_handle = CreateWindow(*m_className, *m_caption, m_style, 5, 5,
				wndRect.right - wndRect.left, wndRect.bottom - wndRect.top,
				0, 0, hinst, (LPVOID)this);
			assert(m_handle);

			// Set pointer to this object in user data of window to allow for window message handling
			SetWindowLongPtr(m_handle, GWLP_USERDATA, (LONG_PTR)this);

			// Setup input handling
			RegisterRawInput();
		}
		~Window_Impl()
		{
			if(m_handle)
			{
				// Unregister raw input
				RAWINPUTDEVICE dev;
				dev.usUsagePage = 1; // Generic Desktop Controls
				dev.usUsage = 6; // Keyboard
				dev.dwFlags = RIDEV_REMOVE;
				dev.hwndTarget = m_handle;

				RegisterRawInputDevices(&dev, 1, sizeof(RAWINPUTDEVICE));

				DestroyWindow(m_handle);
				UnregisterClass((LPCWSTR)&m_wndClass, 0);
			}
		}

		void AdjustWindowRect(RECT& r)
		{
			if(!m_customWindowStyle.enabled)
			{
				::AdjustWindowRect(&r, m_style, false);
			}
			else
			{
				r.left -= (long)m_customWindowStyle.borderMargin.left;
				r.top -= (long)m_customWindowStyle.borderMargin.top;
				r.right += (long)m_customWindowStyle.borderMargin.right;
				r.bottom += (long)m_customWindowStyle.borderMargin.bottom;
			}
		}

		void SetWindowSize(const Vector2i& size)
		{
			// Resize window to client size
			RECT wndRect = { 0, 0, size.x, size.y };
			AdjustWindowRect(wndRect);
			SetWindowPos(m_handle, 0, 0, 0, wndRect.right - wndRect.left, wndRect.bottom - wndRect.top, SWP_NOMOVE | SWP_NOZORDER);
			m_clntSize = size;
		}
		Vector2i GetWindowSize() const
		{
			RECT rect;
			GetWindowRect(m_handle, &rect);
			GetClientRect(m_handle, &rect);
			return Vector2i(rect.right - rect.left, rect.bottom - rect.top);
		}

		/* Raw input handling */
		void RegisterRawInput()
		{
			// Enumerate devices
			Vector<RAWINPUTDEVICELIST> deviceList;
			UINT numDevices;
			GetRawInputDeviceList(nullptr, &numDevices, sizeof(RAWINPUTDEVICELIST));
			if(numDevices == 0)
			{
				Log("No raw input devices found", Logger::Warning);
			}
			deviceList.resize(numDevices);
			GetRawInputDeviceList(deviceList.data(), &numDevices, sizeof(RAWINPUTDEVICELIST));

			Vector<RAWINPUTDEVICE> devicesToUse;

			Logf("Listing %d raw input devices:", Logger::Info, numDevices);
			for(uint32 i = 0; i < numDevices; i++)
			{
				// Device name
				char nameData[1024];
				uint32 dataLen = sizeof(nameData) - 1;
				GetRawInputDeviceInfoA(deviceList[i].hDevice, RIDI_DEVICENAME, nameData, &dataLen);

				// Device Info
				RID_DEVICE_INFO devInfo = { 0 };
				devInfo.cbSize = sizeof(RID_DEVICE_INFO);
				dataLen = sizeof(RID_DEVICE_INFO);
				GetRawInputDeviceInfo(deviceList[i].hDevice, RIDI_DEVICEINFO, &devInfo, &dataLen);

				Logf("	-%s", Logger::Info, nameData);

				if(devInfo.dwType == RIM_TYPEKEYBOARD)
				{
				}
			}

			RAWINPUTDEVICE& dev = devicesToUse.AddZeroed();
			dev.usUsagePage = 1; // Generic Desktop Controls
			dev.usUsage = 6; // Keyboard
			dev.hwndTarget = m_handle;

			if(!RegisterRawInputDevices(devicesToUse.data(), (uint32)devicesToUse.size(), sizeof(RAWINPUTDEVICE)))
			{
				Log("Failed to register raw input devices", Logger::Error);
			}
		}
		void HandleKeyEvent(uint8 code, uint8 newState)
		{
			if(newState == 1)
			{
				outer.OnKeyPressed.Call(m_keyMapping.Translate(code));
			}
			else
			{
				outer.OnKeyReleased.Call(m_keyMapping.Translate(code));
			}
		}
		void HandleRawInput(WPARAM wp, LPARAM lp)
		{

			uint32 size = 0;
			bool gotData = GetRawInputData((HRAWINPUT)lp, RID_INPUT, 0, &size, sizeof(RAWINPUTHEADER)) == 0;
			if(!gotData)
				assert(false);
			RAWINPUT* input = (RAWINPUT*)new uint8[size];

			// Get the data
			GetRawInputData((HRAWINPUT)lp, RID_INPUT, input, &size, sizeof(RAWINPUTHEADER));

			if(input->header.dwType == RIM_TYPEKEYBOARD)
			{
				auto& kb = input->data.keyboard;
				uint8 keyCode = kb.VKey & 0xFF;
				uint8 newState = (kb.Flags & RI_KEY_BREAK) ? 0 : 1;
				uint8& currentState = m_keyStates[keyCode];
				if(currentState != newState)
				{
					currentState = newState;
					HandleKeyEvent(keyCode, newState);
				}
			}

			delete[](uint8*)input;
		}

		/* Window show hide, positioning, etc.*/
		void Show()
		{
			ShowWindow(m_handle, SW_SHOW);
		}
		void Hide()
		{
			ShowWindow(m_handle, SW_HIDE);
		}
		void SetCaption(const WString& cap)
		{
			m_caption = cap;
			::SetWindowText(m_handle, *m_caption);
		}

		// Update loop
		bool Update()
		{
			MSG m;
			while(PeekMessage(&m, m_handle, 0, 0, PM_REMOVE))
			{
				TranslateMessage(&m);
				DispatchMessage(&m);
			}
			return !m_closed;
		}

		void EraseBG(HDC dc)
		{
			RECT wndRect;
			GetWindowRect(m_handle, &wndRect);

			HBRUSH brush1 = CreateSolidBrush(RGB(0, 0, 0));
			HGDIOBJ holdBrush = SelectObject(dc, brush1);

			Rectangle(dc, 0, 0, RECTWIDTH(wndRect) + 1, RECTHEIGHT(wndRect) + 1);

			SelectObject(dc, holdBrush);
			DeleteObject(brush1);

		}
		void PaintTitleBar(HDC dc)
		{
			CustomWindowStyle& cws = m_customWindowStyle;

			RECT wndRect;
			GetWindowRect(m_handle, &wndRect);
			RECT titleBarRect = { 0 };
			titleBarRect.right = RECTWIDTH(wndRect) + 1;
			titleBarRect.bottom = cws.borderMargin.top;

			HBRUSH brush1 = CreateSolidBrush(RGB(255, 255, 255));
			HGDIOBJ holdBrush = SelectObject(dc, brush1);

			Rectangle(dc, titleBarRect.left, titleBarRect.top, titleBarRect.right, titleBarRect.bottom);

			SelectObject(dc, holdBrush);
			DeleteObject(brush1);
		}
		void PaintBorders(HDC dc)
		{
			CustomWindowStyle& cws = m_customWindowStyle;

			RECT wndRect;
			GetWindowRect(m_handle, &wndRect);

			// null pen
			HPEN pen = CreatePen(PS_NULL, 1, RGB(0, 0, 0));
			HGDIOBJ holdPen = SelectObject(dc, pen);

			// Set brush color
			HBRUSH brush = CreateSolidBrush(m_active ? RGB(120, 120, 130) : RGB(20, 20, 20));
			HGDIOBJ holdBrush = SelectObject(dc, brush);

			// Top / Bottom border
			Rectangle(dc, 0, 0, RECTWIDTH(wndRect) + 1, cws.borderMargin.top);
			Rectangle(dc, 0, RECTHEIGHT(wndRect) - cws.borderMargin.bottom, RECTWIDTH(wndRect) + 1, RECTHEIGHT(wndRect));

			// L / R Border
			Rectangle(dc, 0, 0, cws.borderMargin.left, RECTHEIGHT(wndRect));
			Rectangle(dc, RECTWIDTH(wndRect) - cws.borderMargin.right, 0, RECTWIDTH(wndRect) + 1, RECTHEIGHT(wndRect));

			PaintTitleBar(dc);

			SelectObject(dc, holdPen);
			SelectObject(dc, holdBrush);
			DeleteObject(pen);
			DeleteObject(brush);
		}
		void NCActivate(bool active)
		{
			m_active = active;
			HDC dc = GetDCEx(m_handle, 0, DCX_WINDOW | DCX_USESTYLE);
			PaintBorders(dc);
			ReleaseDC(m_handle, dc);
		}

		CustomWindowStyle m_customWindowStyle;
		Ref<HBitmap> m_border;

		// Window Input State
		uint8 m_keyStates[256] = { 0 };
		KeyMap m_keyMapping;

		// Various window state
		bool m_active = true;
		bool m_closed = false;
		bool m_fullscreen = false;
		RECT m_preFullscreenRect;
		uint32 m_style;
		Vector2i m_clntSize;
		WString m_caption;
		WString m_className;
		HWND m_handle;
		WNDCLASSEX m_wndClass;
	};

	// The global window procedure
	//	this function redirects messages to the owning Window_Impl by using the user data of the window as a pointer to that object
	LRESULT CALLBACK WndProc(HWND wnd, UINT msg, WPARAM wp, LPARAM lp)
	{
		if(msg == WM_CREATE)
		{
			CREATESTRUCT* s = (CREATESTRUCT*)lp;
			Window_Impl* impl = (Window_Impl*)s->lpCreateParams;

			// Set pointer to this object in user data of window to allow for window message handling
			SetWindowLongPtr(wnd, GWLP_USERDATA, (LONG_PTR)impl);
			impl->m_handle = wnd;

			// Send frame changed event
			RECT rcClient;
			GetWindowRect(wnd, &rcClient);

			// Inform the application of the frame change.
			if(impl->m_customWindowStyle.enabled)
			{
				SetWindowPos(wnd,
					nullptr,
					rcClient.left, rcClient.top,
					RECTWIDTH(rcClient), RECTHEIGHT(rcClient),
					SWP_FRAMECHANGED);

				SetWindowTheme(wnd, L"", L"");
			}

			// Load custom window border bitmaps
			if(impl->m_customWindowStyle.borderImage)
			{
				impl->m_border = Ref<HBitmap>(new HBitmap(impl->m_customWindowStyle.borderImage, GetDC(wnd)));
			}
		}

		Window_Impl* impl = (Window_Impl*)GetWindowLongPtr(wnd, GWLP_USERDATA);

		if(impl)
		{
			// Call delegates
			impl->outer.OnWindowMessage.Call(wnd, msg, wp, lp);

			CustomWindowStyle& cws = impl->m_customWindowStyle;

			if(msg == WM_INPUT)
			{
				impl->HandleRawInput(wp, lp);
			}
			else if(msg == WM_CLOSE)
			{
				impl->m_closed = true;
			}
			else if(msg == WM_SIZE)
			{
				RECT rect;
				rect.right = LOWORD(lp);
				rect.bottom = HIWORD(lp);
				GetClientRect(wnd, &rect);
				impl->outer.OnResized.Call(Vector2i(RECTWIDTH(rect), RECTHEIGHT(rect)));
			}
			else if(msg == WM_ACTIVATE)
			{
				if(cws.enabled)
				{
					MARGINS margins = {
						(int)cws.borderMargin.left,
						(int)cws.borderMargin.right,
						(int)cws.borderMargin.top,
						(int)cws.borderMargin.bottom,
					};

					HRESULT hr = DwmExtendFrameIntoClientArea(wnd, &margins);
				}
			}
			else if((msg == WM_NCCALCSIZE) && (wp == TRUE))
			{
				if(cws.enabled)
				{
					// Calculate new NCCALCSIZE_PARAMS based on custom NCA inset.
					NCCALCSIZE_PARAMS *pncsp = reinterpret_cast<NCCALCSIZE_PARAMS*>(lp);

					pncsp->rgrc[0].left = pncsp->rgrc[0].left + 0;
					pncsp->rgrc[0].top = pncsp->rgrc[0].top + 0;
					pncsp->rgrc[0].right = pncsp->rgrc[0].right - 0;
					pncsp->rgrc[0].bottom = pncsp->rgrc[0].bottom - 0;

					return 0;
				}
			}
			else if((msg == WM_NCHITTEST))
			{
				if(cws.enabled)
				{
					Vector2i mouse = { GET_X_LPARAM(lp), GET_Y_LPARAM(lp) };

					RECT wndRect;
					GetWindowRect(wnd, &wndRect);


					if(mouse.x < wndRect.left || mouse.x > wndRect.right)
					{
						return HTNOWHERE;
					}
					if(mouse.y < wndRect.top || mouse.y > wndRect.bottom)
					{
						return HTNOWHERE;
					}
					return HTCAPTION;
				}
			}
			else if(msg == WM_PAINT)
			{
				if(cws.enabled)
				{
					RECT wndRect;
					GetWindowRect(wnd, &wndRect);

					PAINTSTRUCT ps;
					HDC dc = BeginPaint(wnd, &ps);
					impl->PaintBorders(dc);
					EndPaint(wnd, &ps);
					return 0;
				}
			}
			else if(msg == WM_NCPAINT)
			{
				if(cws.enabled)
				{
					HDC dc = GetDCEx(wnd, 0, DCX_WINDOW | DCX_USESTYLE);
					impl->PaintBorders(dc);
					ReleaseDC(wnd, dc);
					return 0;
				}
			}
			else if(msg == WM_ERASEBKGND)
			{
				if(cws.enabled)
				{
					HDC dc = (HDC)wp;
					impl->EraseBG(dc);
					return 1;
				}
			}
			else if(msg == WM_NCACTIVATE)
			{
				if(cws.enabled)
				{
					impl->NCActivate(wp != 0);
					return TRUE;
				}
			}
		}
		return DefWindowProc(wnd, msg, wp, lp);
	}

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
		return (void*)m_impl->m_handle;
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
		if(style == WindowStyle::Windowed)
		{
			SetStyles(WS_OVERLAPPEDWINDOW);
		}
		else
		{
			UnsetStyles(WS_OVERLAPPEDWINDOW);
		}
	}
	Vector2i Window::GetWindowSize()
	{
		return m_impl->GetWindowSize();
	}
	void Window::SetWindowSize(const Vector2i& size)
	{
		m_impl->SetWindowSize(size);
	}
	BOOL CALLBACK MonitorEnumCallback(HMONITOR hMonitor, HDC hdcMonitor, LPRECT lprcMonitor, LPARAM dwData)
	{
		Vector<RECT>& out = *(Vector<RECT>*)dwData;
		out.Add(*lprcMonitor);
		return true;
	}
	void Window::SwitchFullscreen(uint32 monitorID)
	{
		if(!m_impl->m_fullscreen)
		{
			RECT rect;
			GetWindowRect(m_impl->m_handle, &rect);
			RECT monitorRect;

			if(monitorID == -1)
			{
				// Get monitor from active window position
				HMONITOR monitor = MonitorFromRect(&rect, MONITOR_DEFAULTTONEAREST);
				m_impl->m_preFullscreenRect = rect;

				MONITORINFO info = { 0 };
				info.cbSize = sizeof(MONITORINFO);
				GetMonitorInfo(monitor, &info);

				monitorRect = info.rcMonitor;
			}
			else
			{
				//Get monitor by index
				Vector<RECT> monitors;
				bool ok = EnumDisplayMonitors(0, 0, &MonitorEnumCallback, (LPARAM)&monitors) == TRUE;
				assert(ok && monitors.size() > 0);

				// Clamp index
				monitorID = Math::Clamp<uint32>(monitorID, 0, (uint32)monitors.size() - 1);
				monitorRect = monitors[monitorID];
			}

			SetWindowStyle(WindowStyle::Borderless);
			int32 sizex = monitorRect.right - monitorRect.left;
			int32 sizey = monitorRect.bottom - monitorRect.top;
			SetWindowPos(m_impl->m_handle, 0, monitorRect.left, monitorRect.top,
				sizex, sizey, SWP_SHOWWINDOW);
			m_impl->m_fullscreen = true;
		}
		else
		{
			// Restore original window style and position
			const RECT& rect = m_impl->m_preFullscreenRect;
			SetWindowStyle(WindowStyle::Windowed);
			int32 sizex = rect.right - rect.left;
			int32 sizey = rect.bottom - rect.top;
			SetWindowPos(m_impl->m_handle, 0, rect.left, rect.top,
				sizex, sizey, 0);
			m_impl->m_fullscreen = false;
		}
	}
	void Window::SetStyles(uint32 mask)
	{
		LONG style = GetWindowLong(m_impl->m_handle, GWL_STYLE);
		style |= mask;
		SetWindowLong(m_impl->m_handle, GWL_STYLE, style);
	}
	void Window::UnsetStyles(uint32 mask)
	{
		LONG style = GetWindowLong(m_impl->m_handle, GWL_STYLE);
		mask = ~mask;
		style &= mask; // Apply mask to disable only given styles
		SetWindowLong(m_impl->m_handle, GWL_STYLE, style);
	}
	uint32 Window::HasStyle(uint32 mask)
	{
		LONG style = GetWindowLong(m_impl->m_handle, GWL_STYLE);
		return style & mask;
	}
	void Window::IsKeyPressed(Key key) const
	{

	}
}