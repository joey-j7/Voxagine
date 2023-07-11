#pragma once

#include <functional>

#include <Core/ECS/WorldManager.h>

class WorldManager;
class World;

template <typename... Args>
struct InputStateCallback
{
private:
	WorldManager* m_pWorldManager = nullptr;
public:
	World* m_pRegisteredWorld = nullptr;
	std::function<void(Args...)> m_fCallback;

	InputStateCallback() = default;
	InputStateCallback(WorldManager* pWorldManager, std::function<void(Args...)> fCallback)
		: m_pWorldManager(pWorldManager)
		, m_pRegisteredWorld(pWorldManager->GetTopWorld())
		, m_fCallback(fCallback)
	{ }

	void operator()(Args... args) const
	{
		if (m_pRegisteredWorld && m_pWorldManager && m_pRegisteredWorld != m_pWorldManager->GetTopWorld())
			return;

		m_fCallback(args...);
	};
};