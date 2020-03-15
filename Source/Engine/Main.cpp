#include "Engine.h"

#define NOMINMAX

#include "DX12/DX12Device.h"
#include "DX12/DX12SwapChain.h"

#include <algorithm>
#include <chrono>

// Window handle.
HWND g_hWnd;
// Window rectangle (used to toggle fullscreen state).
RECT g_WindowRect;

uint32 g_ClientWidth	= 1280;
uint32 g_ClientHeight	= 720;

// By default, use windowed mode.
// Can be toggled with the Alt+Enter or F11
bool g_Fullscreen = false;

DX12Device g_Device;

// Window callback function.
LRESULT CALLBACK WndProc(HWND, UINT, WPARAM, LPARAM);

void RegisterWindowClass(HINSTANCE inIstance, const wchar_t* inWindowClassName)
{
	// Register a window class for creating our render window with.
	WNDCLASSEXW window_class = {};

	window_class.cbSize			= sizeof(WNDCLASSEX);
	window_class.style			= CS_HREDRAW | CS_VREDRAW;
	window_class.lpfnWndProc	= &WndProc;
	window_class.cbClsExtra		= 0;
	window_class.cbWndExtra		= 0;
	window_class.hInstance		= inIstance;
	window_class.hIcon			= ::LoadIcon(inIstance, NULL);
	window_class.hCursor		= ::LoadCursor(NULL, IDC_ARROW);
	window_class.hbrBackground	= (HBRUSH) (COLOR_WINDOW + 1);
	window_class.lpszMenuName	= NULL;
	window_class.lpszClassName	= inWindowClassName;
	window_class.hIconSm		= ::LoadIcon(inIstance, NULL);

	static ATOM atom = ::RegisterClassExW(&window_class);
	Assert(atom > 0);
}

// In order to define a function called CreateWindow, the Windows macro needs to
// be undefined.
#if defined(CreateWindow)
#undef CreateWindow
#endif

HWND CreateWindow(const wchar_t* inWindowClassName, HINSTANCE inInstance,
				  const wchar_t* inWindowTitle, uint32 inWidth, uint32 inHeight)
{
	int screen_width	= ::GetSystemMetrics(SM_CXSCREEN);
	int screen_height	= ::GetSystemMetrics(SM_CYSCREEN);

	RECT windowRect = { 0, 0, static_cast<LONG>(inWidth), static_cast<LONG>(inHeight) };
	::AdjustWindowRect(&windowRect, WS_OVERLAPPEDWINDOW, FALSE);

	int window_width	= windowRect.right - windowRect.left;
	int window_height	= windowRect.bottom - windowRect.top;

	// Center the window within the screen. Clamp to 0, 0 for the top-left corner.
	int windowX = Math::Max(0, (screen_width - window_width) / 2);
	int windowY = Math::Max(0, (screen_height - window_height) / 2);

	HWND h_window = ::CreateWindowExW(
		NULL,
		inWindowClassName,
		inWindowTitle,
		WS_OVERLAPPEDWINDOW,
		windowX,
		windowY,
		window_width,
		window_height,
		NULL,
		NULL,
		inInstance,
		nullptr
	);

	Assert(h_window, "Failed to create window");

	return h_window;
}

void SetFullscreen(bool inFullscreen)
{
	if (g_Fullscreen != inFullscreen)
	{
		g_Fullscreen = inFullscreen;

		// Switching to fullscreen.
		if (g_Fullscreen)
		{
			// Store the current window dimensions so they can be restored 
			// when switching out of fullscreen state.
			::GetWindowRect(g_hWnd, &g_WindowRect);

			// Set the window style to a borderless window so the client area fills
			// the entire screen.
			UINT window_style = WS_OVERLAPPEDWINDOW & ~(WS_CAPTION | WS_SYSMENU | WS_THICKFRAME | WS_MINIMIZEBOX | WS_MAXIMIZEBOX);

			::SetWindowLongW(g_hWnd, GWL_STYLE, window_style);

			// Query the name of the nearest display device for the window.
			// This is required to set the fullscreen dimensions of the window
			// when using a multi-monitor setup.
			HMONITOR h_monitor = ::MonitorFromWindow(g_hWnd, MONITOR_DEFAULTTONEAREST);
			MONITORINFOEX monitor_info = {};
			monitor_info.cbSize = sizeof(MONITORINFOEX);
			::GetMonitorInfo(h_monitor, &monitor_info);

			::SetWindowPos(g_hWnd, HWND_TOP,
						   monitor_info.rcMonitor.left,
						   monitor_info.rcMonitor.top,
						   monitor_info.rcMonitor.right - monitor_info.rcMonitor.left,
						   monitor_info.rcMonitor.bottom - monitor_info.rcMonitor.top,
						   SWP_FRAMECHANGED | SWP_NOACTIVATE);

			::ShowWindow(g_hWnd, SW_MAXIMIZE);
		}
		else
		{
			// Restore all the window decorators.
			::SetWindowLong(g_hWnd, GWL_STYLE, WS_OVERLAPPEDWINDOW);

			::SetWindowPos(g_hWnd, HWND_NOTOPMOST,
						   g_WindowRect.left,
						   g_WindowRect.top,
						   g_WindowRect.right - g_WindowRect.left,
						   g_WindowRect.bottom - g_WindowRect.top,
						   SWP_FRAMECHANGED | SWP_NOACTIVATE);

			::ShowWindow(g_hWnd, SW_NORMAL);
		}
	}
}

#include "test.h"

void Update()
{
	static uint64 frame_counter		= 0;
	static double elapsed_seconds	= 0.0;
	static std::chrono::high_resolution_clock clock;
	static auto t0					= clock.now();

	frame_counter++;
	auto t1 = clock.now();
	auto delta_time = t1 - t0;
	t0 = t1;

	elapsed_seconds += delta_time.count() * 1e-9;
	if (elapsed_seconds > 1.0)
	{
		auto fps = frame_counter / elapsed_seconds;
		Trace("FPS: %f", fps);

		frame_counter = 0;
		elapsed_seconds = 0.0;
	}

	float deltaMS = static_cast<float>(delta_time.count() * 1e-6);

	OnUpdate(g_ClientWidth, g_ClientHeight, deltaMS);
}

void Resize(uint32_t inWwidth, uint32_t inHeight)
{
	if (g_ClientWidth != inWwidth || g_ClientHeight != inHeight)
	{
		// Don't allow 0 size swap chain back buffers.
		g_ClientWidth	= Math::Max(1u, inWwidth);
		g_ClientHeight	= Math::Max(1u, inHeight);

		g_Device.ResestDescriptorHeaps();

		g_Device.m_SwapChain->UpdateRenderTargetViews(g_Device, g_ClientWidth, g_ClientHeight);
	}

	OnResize(g_Device, inWwidth, inHeight);
}

LRESULT CALLBACK WndProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	switch (message)
	{
	case WM_PAINT:
		Update();
		OnRender(g_Device);
		break;
	case WM_SYSKEYDOWN:
	case WM_KEYDOWN:
	{
		bool alt = (::GetAsyncKeyState(VK_MENU) & 0x8000) != 0;

		switch (wParam)
		{
		case VK_ESCAPE:
			::PostQuitMessage(0);
			break;
		case VK_RETURN:
			if (alt)
			{
		case VK_F11:
			SetFullscreen(!g_Fullscreen);
			}
			break;
		}
	}
	break;
	// The default window procedure will play a system notification sound 
	// when pressing the Alt+Enter keyboard combination if this message is 
	// not handled.
	case WM_SYSCHAR:
		break;
	case WM_SIZE:
	{
		RECT clientRect = {};
		::GetClientRect(g_hWnd, &clientRect);

		int width	= clientRect.right - clientRect.left;
		int height	= clientRect.bottom - clientRect.top;

		Resize(width, height);
	}
	break;
	case WM_DESTROY:
		::PostQuitMessage(0);
		break;
	default:
		return ::DefWindowProcW(hwnd, message, wParam, lParam);
	}

	return 0;
}

int CALLBACK wWinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, PWSTR lpCmdLine, int nCmdShow)
{
	// Windows 10 Creators update adds Per Monitor V2 DPI awareness context.
	// Using this awareness context allows the client area of the window 
	// to achieve 100% scaling while still allowing non-client window content to 
	// be rendered in a DPI sensitive fashion.
	SetThreadDpiAwarenessContext(DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2);

	// Window class name. Used for registering / creating the window.
	const wchar_t* window_class_name = L"AmigoDX12WindowClass";
	//ParseCommandLineArguments();

	RegisterWindowClass(hInstance, window_class_name);
	g_hWnd = CreateWindow(window_class_name, hInstance, L"Amigo engine DX12",
						  g_ClientWidth, g_ClientHeight);

	// Initialize the global window rect variable.
	::GetWindowRect(g_hWnd, &g_WindowRect);

	g_Device.Init(g_hWnd, g_ClientWidth, g_ClientHeight);

	LoadContent(g_Device, g_ClientWidth, g_ClientHeight);

	::ShowWindow(g_hWnd, SW_SHOW);

	MSG msg = {};
	while (msg.message != WM_QUIT)
	{
		if (::PeekMessage(&msg, NULL, 0, 0, PM_REMOVE))
		{
			::TranslateMessage(&msg);
			::DispatchMessage(&msg);
		}
	}

	UnloadContent(g_Device);

	return 0;
}