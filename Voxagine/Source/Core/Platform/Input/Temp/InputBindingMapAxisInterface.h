#pragma once

#include <string>
#include <vector>
#include <functional>

#include "InputKeyIdentifiers.h"

class InputBindingAxis;

struct AxisMapInformation
{
	std::string MapName;
	std::vector<InputBindingAxis>* AxisMap;
};

class InputBindingMapAxisInterface
{
public:
	InputBindingMapAxisInterface() {};
	virtual ~InputBindingMapAxisInterface() {};

	void RegisterAxis(const std::string& axisName, const InputKey& masterKey, float fScalar = 1.f);
	void RegisterAxis(const std::string& mapName, const std::string& axisName, const InputKey& masterKey, float fScalar = 1.f);

	uint64_t BindAxis(const std::string& axisName, std::function<void(float)> bindFunc);
	uint64_t BindAxis(const std::string& mapName, const std::string& axisName, std::function<void(float)> bindFunc);
	bool UnBindAxis(uint64_t bindHandle);

protected:
	InputBindingAxis* FindAxis(const std::string& axisName);
	InputBindingAxis* FindAxis(const std::string& mapName, const std::string& axisName);
	
	std::vector<InputBindingAxis>* FindAxisMap();
	std::vector<InputBindingAxis>* FindAxisMap(const std::string& mapName);
private:
	virtual const std::string& GetDefaultMapName() const = 0;
	virtual uint64_t GetInvalidBindHandle() const = 0;
	virtual void GetAllAxisMaps(std::vector<AxisMapInformation>& axisMapInformations) = 0;
};