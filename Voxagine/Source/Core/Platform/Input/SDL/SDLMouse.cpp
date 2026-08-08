#include "pch.h"
#include "SDLMouse.h"

#include <SDL3/SDL.h>

Mouse* Mouse::s_pInstance = nullptr;

Mouse::Mouse()
{
	s_pInstance = this;
}

Mouse::~Mouse()
{
	if (s_pInstance == this)
		s_pInstance = nullptr;
}

Mouse& Mouse::Get()
{
	if (s_pInstance == nullptr)
		new Mouse();

	return *s_pInstance;
}

void Mouse::AddScrollDelta(float fDelta)
{
	/* DirectXTK counted in WHEEL_DELTA units of 120 per notch; SDL reports
	   notches. Scaled so anything tuned against the old values still works. */
	m_iScrollWheelValue += static_cast<int>(fDelta * 120.f);
}

Mouse::State Mouse::GetState() const
{
	State state;

	float fX = 0.f;
	float fY = 0.f;
	const SDL_MouseButtonFlags buttons = SDL_GetMouseState(&fX, &fY);

	state.x = static_cast<int>(fX);
	state.y = static_cast<int>(fY);

	state.leftButton = (buttons & SDL_BUTTON_LMASK) != 0;
	state.middleButton = (buttons & SDL_BUTTON_MMASK) != 0;
	state.rightButton = (buttons & SDL_BUTTON_RMASK) != 0;
	state.xButton1 = (buttons & SDL_BUTTON_X1MASK) != 0;
	state.xButton2 = (buttons & SDL_BUTTON_X2MASK) != 0;

	state.scrollWheelValue = m_iScrollWheelValue;

	return state;
}
