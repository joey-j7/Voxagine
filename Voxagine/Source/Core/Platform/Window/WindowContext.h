#pragma once

#include <stdint.h>
#include "Core/Math.h"

#include "Core/Event.h"

class Settings;
class Platform;

class WindowContext
{
public:
	WindowContext(Platform* pPlatform);
	virtual ~WindowContext();

	virtual void Initialize();

	virtual void* GetHandle() = 0;

	const UVector2& GetPosition() const;
	const UVector2& GetSize() const;

	virtual	void Poll() = 0;

	/* Events */
	virtual void OnMove() = 0;
	
	Platform* GetPlatform() const { return m_pPlatform; };

protected:

	/* Events */
	virtual void OnResize(uint32_t a_uiWidth, uint32_t a_uiHeight, IVector2 resolutionDelta) = 0;
	virtual void OnFullscreenChanged(bool bFullscreen) = 0;

	Platform* m_pPlatform;

	UVector2 m_v2Position;
	UVector2 m_v2Size;
};