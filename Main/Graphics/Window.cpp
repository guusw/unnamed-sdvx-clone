#include "stdafx.h"
#include "Window.hpp"
#include <Uxtheme.h>
#include <dwmapi.h>
#pragma comment(lib, "dwmapi.lib")

static LRESULT CALLBACK WndProc(HWND wnd, UINT msg, WPARAM wp, LPARAM lp);
static HINSTANCE hinst = GetModuleHandle(nullptr);
static int wndIdx = 0;

class Window_Impl
{
public:
	// Handle to outer class to send delegates
	Window& outer;
public:
	Window_Impl(Window& outer, Vector2i size) : outer(outer)
	{
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
		AdjustWindowRect(&wndRect, m_style, false);

		// Create the window and make sure it was successfull
		m_handle = CreateWindow(*m_className, *m_caption, m_style, -1, -1, 
			wndRect.right - wndRect.left, wndRect.bottom - wndRect.top,
			0, 0, hinst, 0);
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

	void SetWindowSize(const Vector2i& size)
	{
		// Resize window to client size
		RECT wndRect = { 0, 0, size.x, size.y };
		AdjustWindowRect(&wndRect, m_style, false);
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
			outer.OnKeyPressed.Call(code);
		}
		else
		{
			outer.OnKeyReleased.Call(code);
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
			uint8& currentState = keyStates[keyCode];
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

	// Window Input State
	uint8 keyStates[256] = { 0 };

	// Various window state
	bool m_closed = false;
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
	Window_Impl* impl = (Window_Impl*)GetWindowLongPtr(wnd, GWLP_USERDATA);
	if(impl)
	{
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
			impl->outer.OnResized.Call(impl->GetWindowSize());
		}
	}
	return DefWindowProc(wnd, msg, wp, lp);
}

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

Vector2i Window::GetWindowSize()
{
	return m_impl->GetWindowSize();
}
void Window::SetWindowSize(const Vector2i& size)
{
	m_impl->SetWindowSize(size);
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

