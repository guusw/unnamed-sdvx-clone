#include "stdafx.h"
#include "Window.hpp"
#ifndef _WIN32
class Window_Impl
{
public:
	// Handle to outer class to send delegates
	DesktopWindow& outer;
public:
	Window_Impl(DesktopWindow& outer, Vector2i size, const CustomWindowStyle& customStyle) : outer(outer)
	{
		m_caption = L"Window";
	}
	~Window_Impl()
	{
	}

	void SetWindowSize(const Vector2i& size)
	{
		throw std::logic_error("The method or operation is not implemented.");
	}
	Vector2i GetWindowSize()
	{
		throw std::logic_error("The method or operation is not implemented.");
	}
	void SetCaption(const WString& cap)
	{
		throw std::logic_error("The method or operation is not implemented.");
	}
	bool Update()
	{
		throw std::logic_error("The method or operation is not implemented.");
	}
	void Hide()
	{
		throw std::logic_error("The method or operation is not implemented.");
	}
	void Show()
	{
		throw std::logic_error("The method or operation is not implemented.");
	}

	bool m_closed = false;
	Window m_window;
	Display m_display;
	WString m_caption;
}

DesktopWindow::DesktopWindow(Vector2i size, const CustomWindowStyle& customStyle)
{
	m_impl = new Window_Impl(*this, size, customStyle);
}
DesktopWindow::~DesktopWindow()
{
	delete m_impl;
}
void DesktopWindow::Show()
{
	m_impl->Show();
}
void DesktopWindow::Hide()
{
	m_impl->Hide();
}
bool DesktopWindow::Update()
{
	return m_impl->Update();
}
void* DesktopWindow::Handle()
{
	return (void*)m_impl->m_window;
}
void DesktopWindow::SetCaption(const WString& cap)
{
	m_impl->SetCaption(cap);
}
void DesktopWindow::Close()
{
	m_impl->m_closed = true;
}
void DesktopWindow::SetWindowStyle(WindowStyle style)
{
}
Vector2i DesktopWindow::GetWindowSize()
{
	return m_impl->GetWindowSize();
}
void DesktopWindow::SetWindowSize(const Vector2i& size)
{
	m_impl->SetWindowSize(size);
}
void DesktopWindow::SwitchFullscreen(uint32 monitorID)
{
}
void DesktopWindow::SetStyles(uint32 mask)
{
}
void DesktopWindow::UnsetStyles(uint32 mask)
{
}
uint32 DesktopWindow::HasStyle(uint32 mask)
{
	return 0;
}

#endif