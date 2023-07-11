#pragma once

class ImPlatform {
public:
	virtual void Initialize() = 0;
	virtual void NewFrame() = 0;
};