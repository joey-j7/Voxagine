#pragma once
#include <stack>
#include <functional>
#include <queue>
#include <string>
#include "Core/Event.h"

class World;
class Application;
class WorldManager
{
public:
	WorldManager(Application* pApp);
	~WorldManager();

	Event<World*> WorldLoaded;
	Event<World*> WorldPopped;

	void ClearWorlds();

	void LoadWorld(World* pWorld);
	void PushWorld(World* pWorld);

	void PopWorld();
	void SwapWorlds();

	bool RequiresSwap() { return m_DeferredFuncs.size() > 0; }
	World* GetTopWorld();

	size_t GetWorldCount() const { return m_Worlds.size(); };
	const std::vector<World*>& GetWorlds() const { return m_Worlds; }

	std::vector<std::string> GetWorldFiles() const { return m_WorldFiles; }
	void SetWorldFiles(std::vector<std::string> m_vFiles) { m_WorldFiles = std::move(m_vFiles); }

	const std::string GetPreviousWorldName() const { return m_sPreviousWorld; }

private:
	Application* m_pApplication;
	std::vector<World*> m_Worlds;
	std::queue<std::function<void()>> m_DeferredFuncs;

	std::vector<std::string> m_WorldFiles;
	std::string m_sPreviousWorld;
};