#pragma once

// TODO: Replace this with a Vec2
struct MousePos
{
	float x = 0.0f;
	float y = 0.0f;
};

enum class MouseButton
{
	Left = 0,
	Right,
	Middle,
	Count
};

struct ButtonState
{
	bool		m_IsDown				= false;
	bool		m_WasPressed			= false;
	MousePos	m_NormalizedPosAtClick	= { 0.0f, 0.0f };
};

class Mouse
{
protected:
	MousePos	m_CurrentPos;
	MousePos	m_NormalizedPos;
	MousePos	m_WindowSize;

	ButtonState	m_ButtonStates[(int) MouseButton::Count];

public:
	void		Update();
	void		UpdateWindowSize(int inWidth, int inHeight);

	// Mouse coords in [0..m_WindowSize] range, (0, 0) is the top left corner
	MousePos	GetCurrentPos() const;

	// Mouse coords in [-1..1] range, (0, 0) is the center of the screen
	MousePos	GetNormalizedPos() const;

	// Get normalized coords of a button click
	MousePos	GetNormalizedClickPos(MouseButton inButton) const;

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