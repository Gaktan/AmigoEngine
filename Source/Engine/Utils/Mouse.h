#pragma once

#include "Math/Math.h"

enum class MouseButton
{
	Left = 0,
	Right,
	Middle,
	Count
};

struct ButtonState
{
	bool	m_IsDown				= false;
	bool	m_WasPressed			= false;
	Vec2	m_NormalizedPosAtClick	= { 0.0f, 0.0f };
};

class Mouse
{
protected:
	Vec2	m_CurrentPos;
	Vec2	m_NormalizedPos;
	Vec2	m_WindowSize;

	ButtonState	m_ButtonStates[(int) MouseButton::Count];

public:
	void		Update();
	void		UpdateWindowSize(int inWidth, int inHeight);

	// Mouse coords in [0..m_WindowSize] range, (0, 0) is the top left corner
	Vec2		GetCurrentPos() const;

	// Mouse coords in [-1..1] range, (0, 0) is the center of the screen
	Vec2		GetNormalizedPos() const;

	// Get normalized coords of a button click
	Vec2		GetNormalizedClickPos(MouseButton inButton) const;

	// Returns true if the passed button is currently pressed this frame
	bool		IsButtonDown(MouseButton inButton) const;

	// Returns true if the passed button was pressed last frame AND the passed button was released this frame
	bool		WasJustReleased(MouseButton inButton) const;

protected:
	void		UpdatePos(int inMousePosX, int inMousePosY);

	// Static members
public:
	static Mouse& GetInstance();
};