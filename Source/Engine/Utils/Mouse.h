#pragma once

// TODO: Replace this with a Vec2
struct MousePos
{
	float x = 0.0f;
	float y = 0.0f;
};

enum MouseButton
{
	Left = 0,
	Right,
	Middle,
	Count
};

struct ButtonState
{
	bool		m_Down					= false;
	MousePos	m_NormalizedPosAtClick	= { 0.0f, 0.0f };
};

class Mouse
{
protected:
	MousePos	m_CurrentPos;
	MousePos	m_NormalizedPos;
	MousePos	m_WindowSize;

	ButtonState	m_ButtonStates[MouseButton::Count];

public:
	void		HandleMouseEvents(uint64 inMessage, uint64 inWParam, int64 inLParam);
	void		UpdateMousePos(int inMousePosX, int inMousePosY);
	void		UpdateWindowSize(int inWidth, int inHeight);

	// Mouse Coords in [0..m_WindowSize] range, (0, 0) is the top left corner
	MousePos	GetCurrentPos() const;

	// Mouse Coords in [-1..1] range, (0, 0) is the center of the screen
	MousePos	GetNormalizedPos() const;

	MousePos	GetNormalizedClickPos(MouseButton inButton) const;

	bool		IsButtonDown(MouseButton inButton) const;

	// Static members
public:
	static Mouse& GetInstance();
};