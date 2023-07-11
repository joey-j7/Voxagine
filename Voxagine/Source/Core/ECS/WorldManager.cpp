#include "pch.h"
#include "Core/ECS/WorldManager.h"

#include "Core/Application.h"
#include "Core/ECS/World.h"
#include "External/optick/optick.h"

WorldManager::WorldManager(Application* pApp)
{
	m_pApplication = pApp;
}

WorldManager::~WorldManager()
{
	ClearWorlds();
}

void WorldManager::ClearWorlds()
{
	while (!m_Worlds.empty())
	{
		World* pWorld = m_Worlds.back();
		WorldPopped(pWorld);

		pWorld->Unload();

		delete pWorld;
		pWorld = nullptr;
		m_Worlds.pop_back();
	}
}

void WorldManager::LoadWorld(World* pWorld)
{
	m_DeferredFuncs.push([this, pWorld]()
	{
		if (!m_Worlds.empty())
		{
			World* pTopWorld = m_Worlds.back();
			m_sPreviousWorld = pTopWorld->GetName();
			WorldPopped(pTopWorld);

			pTopWorld->Unload();

			delete pTopWorld;
			pTopWorld = nullptr;
			m_Worlds.pop_back();
		}

		m_Worlds.push_back(pWorld);
		pWorld->Initialize();
		WorldLoaded(pWorld);
	});
}

void WorldManager::PushWorld(World* pWorld)
{
	m_DeferredFuncs.push([this, pWorld]()
	{
		m_sPreviousWorld = m_Worlds.back()->GetName();
		m_Worlds.back()->Pause();
		m_Worlds.push_back(pWorld);
		pWorld->Initialize();
		WorldLoaded(pWorld);
	});
}

void WorldManager::PopWorld()
{
	if (m_Worlds.empty()) return;

	m_DeferredFuncs.push([this]()
	{
		World* pWorld = m_Worlds.back();
		m_sPreviousWorld = pWorld->GetName();
		WorldPopped(pWorld);

		pWorld->Unload();

		delete pWorld;
		pWorld = nullptr;
		m_Worlds.pop_back();

		World* oldWorld = m_Worlds.back();
		if (oldWorld)
			oldWorld->Resume();
	});
}

void WorldManager::SwapWorlds()
{
	OPTICK_EVENT();
	while (!m_DeferredFuncs.empty())
	{
		std::function<void()>& func = m_DeferredFuncs.front();
		func();
		m_DeferredFuncs.pop();
	}
}

World* WorldManager::GetTopWorld()
{
	if (!m_Worlds.empty())
		return m_Worlds.back();
	return nullptr;
}
