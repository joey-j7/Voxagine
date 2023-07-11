#pragma once

#include "Editor/Window.h"

namespace rttr
{
	class instance;
	class property;
}

class EntityInspector : public Window
{
public:
	EntityInspector();
	~EntityInspector();

	void Initialize(Editor* pTargetEditor) override;
	void UnInitialize() override;

	void Tick(float fDeltaTime) override;

	void OnContextResize(uint32_t a_uiWidth, uint32_t a_uiHeight, IVector2 deltaResolution) override;

	void RenderSelectedEntityProperty(rttr::instance& rObject, rttr::property & rProperty, std::string* pCategory = nullptr);

protected:
	struct PropertyCategory
	{
		rttr::instance Instance;
		rttr::property Property;
		std::string  StringLabel;

		bool bKeepComponentAlive = true;
		bool bIsComponent = false;
		bool bHighPriority = false;
	};

	void OnPreRender(float fDeltaTime) override;
	void OnRender(float fDeltaTime) override;

private:
	void RenderCategories(std::unordered_multimap<std::string, PropertyCategory>& CategoryOrganizerMap);

	std::unordered_multimap<std::string, PropertyCategory> m_CategoryOrganizerMap = { };
	bool m_bRenderDefaultCategory = true;
	bool m_bRenderAddComponentWindow;
	std::string m_SearchStringAddComponent;
};