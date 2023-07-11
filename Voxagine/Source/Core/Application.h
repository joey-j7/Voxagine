#pragma once
class Platform;

#include "ECS/WorldManager.h"
#include "Resources/ResourceManager.h"
#include "Core/Settings.h"
#include "Core/GameTimer.h"
#include "Platform/Platform.h"
#include "Core/JsonSerializer.h"
#include "Core/Threading/JobManager.h"
#include "Core/PlayerPrefs/PlayerPrefs.h"
#include "Core/LoggingSystem/LoggingSystem.h"

#ifdef EDITOR
#include "Editor/Editor.h"
#endif

class FileSystem;
class Application
{
public:
	Application();
	virtual ~Application();

	void Run();
	void Exit() { m_bExit = true; }

	Platform& GetPlatform() { return m_Platform; }
	WorldManager& GetWorldManager() { return m_WorldManager; }
	ResourceManager& GetResourceManager() { return m_ResourceManager; }
	Settings& GetSettings() { return m_Settings; }
	LoggingSystem& GetLoggingSystem() { return m_LoggingSystem; }
	JsonSerializer& GetSerializer() { return m_Serializer; }
	JobManager& GetJobManager() { return m_JobManager; }
	FileSystem* GetFileSystem() { return m_pFileSystem; }
	const GameTimer& GetTimer() const { return *m_Platform.GetGameTimer(); }
	const GameTimer& GetFixedTimer() const { return *m_Platform.GetFixedTimer(); }

	bool IsSuspended() const { return m_bSuspended; }
	void SetSuspended(bool bSuspended) { m_bSuspended = bSuspended; }

	bool IsInEditor() const
	{
#ifdef EDITOR
		return m_Editor.GetEditorModus() == EditorModus::EM_EDITOR;
#else
		return false;
#endif
	}

	bool IsShuttingDown() const { return m_bExit; }

protected:
	virtual void OnCreate() {};
	virtual void OnUpdate() {};
	virtual void OnDraw() {};
	virtual void OnExit() {};

	Platform m_Platform;
	WorldManager m_WorldManager;
	ResourceManager m_ResourceManager;
	Settings m_Settings;
	LoggingSystem m_LoggingSystem;
	JsonSerializer m_Serializer;
	JobManager m_JobManager;
	FileSystem* m_pFileSystem = nullptr;
	PlayerPrefs m_PlayerPrefs;
private:
	void LoadSettings();

#ifdef EDITOR
 	Editor m_Editor;
#endif

	std::atomic_bool m_bSuspended;
	std::atomic_bool m_bExit;
};