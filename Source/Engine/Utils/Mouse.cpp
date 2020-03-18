#include "Engine.h"
#include "Mouse.h"

#include <Windows.h>

#define GET_X_LPARAM(lp) ((int)(short)LOWORD(lp))
#define GET_Y_LPARAM(lp) ((int)(short)HIWORD(lp))

extern HWND g_hWnd;

void Mouse::Update()
{
	Assert(m_WindowSize.x > 0.0f && m_WindowSize.y > 0.0f);

	bool has_focus		= GetFocus() == g_hWnd;
	bool has_capture	= GetCapture() == g_hWnd;

	// Don't update anything if we dont have focus or Capture
	if (!has_focus && !has_capture)
	{
		return;
	}

	// Update mouse position
	POINT cursor_pos;
	GetCursorPos(&cursor_pos);
	ScreenToClient(g_hWnd, &cursor_pos);
	UpdatePos(cursor_pos.x, cursor_pos.y);

	short win_button_states[(int) MouseButton::Count];
	win_button_states[(int) MouseButton::Left]		= GetAsyncKeyState(VK_LBUTTON);
	win_button_states[(int) MouseButton::Right]		= GetAsyncKeyState(VK_RBUTTON);
	win_button_states[(int) MouseButton::Middle]	= GetAsyncKeyState(VK_MBUTTON);

	for (int i = 0; i < (int) MouseButton::Count; i++)
	{
		short win_button_state				= win_button_states[i];
		bool win_button_pressed				= win_button_state != 0;
		bool win_button_pressed_this_frame	= win_button_state & 0x1;

		ButtonState& button_state	= m_ButtonStates[i];
		button_state.m_WasPressed	= false;

		if (win_button_pressed_this_frame)
		{
			// Cursor is outside the window, don't process button press
			if (m_CurrentPos.x < 0.0f || m_CurrentPos.y < 0.0f)
			{
				continue;
			}

			button_state.m_IsDown = true;
			button_state.m_NormalizedPosAtClick	= m_NormalizedPos;

			// Captures mouse input either when the mouse is over the capturing window,
			// or when the mouse button was pressed while the mouse was over the capturing window and the button is still down.
			SetCapture(g_hWnd);
		}
		else if (!win_button_pressed && button_state.m_IsDown)
		{
			button_state.m_IsDown		= false;
			button_state.m_WasPressed	= true;

			// Release captured mouse
			ReleaseCapture();
		}
	}
}

void Mouse::UpdatePos(int inMousePosX, int inMousePosY)
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
	return m_ButtonStates[(int) inButton].m_NormalizedPosAtClick;
}

bool Mouse::IsButtonDown(MouseButton inButton) const
{
	return m_ButtonStates[(int) inButton].m_IsDown;
}

bool Mouse::WasJustReleased(MouseButton inButton) const
{
	return m_ButtonStates[(int) inButton].m_WasPressed;
}

Mouse& Mouse::GetInstance()
{
	static Mouse instance;
	return instance;
}
