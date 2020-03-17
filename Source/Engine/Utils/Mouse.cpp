#include "Engine.h"
#include "Mouse.h"

#include <Windows.h>

#define GET_X_LPARAM(lp) ((int)(short)LOWORD(lp))
#define GET_Y_LPARAM(lp) ((int)(short)HIWORD(lp))

extern HWND g_hWnd;

void Mouse::HandleMouseEvents(uint64 inMessage, uint64 inWParam, int64 inLParam)
{
	// Handle mouse movements
	if (inMessage == WM_MOUSEMOVE)
	{
		int mouse_x = GET_X_LPARAM(inLParam);
		int mouse_y = GET_Y_LPARAM(inLParam);

		UpdateMousePos(mouse_x, mouse_y);
	}
	// Handle button press (Left, Right, Middle click)
	if (inMessage >= WM_LBUTTONDOWN && inMessage <= WM_MBUTTONDBLCLK)
	{
		MouseButton button = MouseButton::Left;
		if (inMessage >= WM_RBUTTONDOWN && inMessage <= WM_RBUTTONDBLCLK)
			button = MouseButton::Right;
		else if (inMessage >= WM_MBUTTONDOWN && inMessage <= WM_MBUTTONDBLCLK)
			button = MouseButton::Middle;

		if (inMessage == WM_LBUTTONDOWN || inMessage == WM_RBUTTONDOWN || inMessage == WM_MBUTTONDOWN)
		{
			m_ButtonStates[button].m_Down					= true;
			m_ButtonStates[button].m_NormalizedPosAtClick	= m_NormalizedPos;

			// Captures mouse input either when the mouse is over the capturing window,
			// or when the mouse button was pressed while the mouse was over the capturing window and the button is still down.
			SetCapture(g_hWnd);
		}
		else if (inMessage == WM_LBUTTONUP || inMessage == WM_RBUTTONUP || inMessage == WM_MBUTTONUP)
		{
			m_ButtonStates[button].m_Down = false;

			// Release captured mouse
			ReleaseCapture();
		}
		else if (inMessage == WM_LBUTTONDBLCLK || inMessage == WM_RBUTTONDBLCLK || inMessage == WM_MBUTTONDBLCLK)
		{
			// Double click not supported yet
		}
	}
}

void Mouse::UpdateMousePos(int inMousePosX, int inMousePosY)
{
	m_CurrentPos = { (float) inMousePosX, (float) inMousePosY };

	m_NormalizedPos.x = (m_CurrentPos.x / m_WindowSize.x) * 2.0f - 1.0f;
	m_NormalizedPos.y = -((m_CurrentPos.y / m_WindowSize.y) * 2.0f - 1.0f);
}

void Mouse::UpdateWindowSize(int inWidth, int inHeight)
{
	m_WindowSize = { (float) inWidth, (float) inHeight };
}

MousePos Mouse::GetCurrentPos() const
{
	return m_CurrentPos;
}

MousePos Mouse::GetNormalizedPos() const
{
	return m_NormalizedPos;
}

MousePos Mouse::GetNormalizedClickPos(MouseButton inButton) const
{
	return m_ButtonStates[inButton].m_NormalizedPosAtClick;
}

bool Mouse::IsButtonDown(MouseButton inButton) const
{
	return m_ButtonStates[inButton].m_Down;
}

Mouse& Mouse::GetInstance()
{
	static Mouse instance;
	return instance;
}
