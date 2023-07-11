#pragma once

#include <vector>

#include "Core/Event.h"
#include "Editor/EditorModus.h"
#include <External/imguizmo/ImGuizmo.h>
#include "Editor/SnappingTool/SnappingTool.h"

#include "Editor/PropertyRenderer/PropertyRenderer.h"
#include "Editor/ConsoleLog/ConsoleLog.h"
#include "Editor/EntityHierarchy/EntityHierarchy.h"
#include "Editor/EntityInspector/EntityInspector.h"
#include "Core/FileBrowser.h"

#include "Editor/Configuration/Project/ProjectConfigurationWindow.h"
#include "Editor/Configuration/Project/ProjectSettings.h"
#include "Editor/Configuration/Editor/EditorConfigurationWindow.h"
#include "Editor/Configuration/Editor/UserSettings.h"
#include "Editor/Configuration/WorldCreateConfig.h"

#include "Editor/EditorReferenceManager.h"

#include "Core/JsonSerializer.h"
#include <External/rapidjson/document.h>

#include <unordered_map>

#include "Core/Math.h"

#define EDITOR_INPUT_LAYER_NAME "Editor"

class Application;
class Entity;
class VoxRenderer;
class World;
class EditorWorld;
class GameTimer;
class Camera;
class TextureReference;
class RenderContext;
class EditorButton;

class Editor
{
public:
	friend class ConsoleLog;
	friend class EntityHierarchy;

	Editor();
	~Editor();

	void Initialize(Application* pTargetApplication);
	void UnInitialize();

	void WorldPreTick(World* pActiveWorld);
	void WorldTick(World* pActiveWorld, float fDeltaTime);
	void WorldPostTick(World* pActiveWorld, float fDeltaTime);

	void WorldFixedTick(World* pActiveWorld, const GameTimer& fixedTimer);
	void WorldPostFixedTick(World* pActiveWorld, const GameTimer& fixedTimer);

	void WorldRender(World* pActiveWorld, const GameTimer& fixedTimer);

	void Render(float fDeltaTime);

	void OnResize(uint32_t a_uiWidth, uint32_t a_uiHeight, IVector2 deltaResolution);

	std::string GeEditorWorld() const { return m_sWorldFilePath; }

	Application* GetApplication();

	void SetSelectedEntity(Entity* pNewSelectedEntity);
	void ResetSelectedEntity();
	bool HasSelectedEntity();
	Entity* GetSelectedEntity();

	bool IsModifyingSelectedEntityTransform() const;
	bool IsMouseHoveringEditorWindows() const;
	bool IsGameWindowHoverered() const;
	EditorModus GetEditorModus() const;

	void SetTransformWorldModus(bool bWorldModus);
	bool IsTransformWorldModus() const;

	PropertyRenderer& GetPropertyRenderer();
	EditorWorld* GetEditorWorld();

	ProjectSettings& GetProjectSettings();
	UserSettings& GetUserSettings();

	bool GetCopyEntityData(Document*& entityCopyDocument, Value*& entityCopyValue);
private:
	void InitializeProjectSettings();
	void LoadResources();
	void InitializeUserSettings();
	void InitializeInput();

	void InitializeButtons();
	void InitializeTextures();

	void SetWorldFilePath(std::string sWorldFilePath);

	void SaveEditorWorld(std::string sFilePath = std::string());
	void UpdateAutoSaveWorld(float fDeltaTime);
	void AutoSaveWorld();

	void OnPlay(bool bAutoSave);
	void OnPause();
	void OnEditor();
	void OnRestart();

	void CreateTemplateWorld(bool bCreateProjectDefault = false, UVector2 chunkGridSize = UVector2(1, 1));

	void RenderGameWindow();
	void RenderMainMenuBar();
	void RenderEntityTransformation();
	void RenderConfigurationWindows(float fDeltaTime);
	void RenderEditorTools(float fDeltaTime);

	void RenderWindowCreateWorld();
	void RenderWindowRenderInfo();
	void RenderWindowWorldSettings();
	void RenderWindowAboutVoxagine();

	void OutlineEntity(Entity* pTargetEntity);
	void DeOutlineEntity(Entity* pTargetEntity);
	void SetOutlineEntity(Entity* pTargetEntity, bool bOutline);

	void SetRenderEditorWindows(bool bRenderEnabled);
	bool GetRenderEditorWindows() const;

	EditorButton* GetButton(const std::string& buttonName);

	// Get the VoxRenderer of target entity, nullptr = false, valid pointer = true
	VoxRenderer* EntityHasComponentVoxRenderer(Entity* pTargetEntity);

	void SetEditorWorld(EditorWorld* pEditorWorld);
	bool HasEditorWorld();

	void SetEditorModus(EditorModus NewEditorModus);

	void OnWorldCreated(World* pCreatedWorld);
	void OnWorldDestroyed(World* pDestroyedWorld);

	void OnEntityCreated(Entity* pCreatedEntity);
	void OnEntityDestroyed(Entity* pDestroyedEntity);

	void FocusUnfocusGameWindow();
	void PossessEjectPlayer();
public:
	Event<Entity*> EntitySelected;
	Event<Entity*> EntityDeSelected;
private:
	Application* m_pApplication = nullptr;
	EditorModus m_EditorModus = EditorModus::EM_EDITOR;
	EditorWorld* m_pEditorWorld = nullptr;

	bool m_bGameFocus = false;
	bool m_bPossessedPlayer = false;
	bool m_bGameWindowFocused = false;
	bool m_bGameWindowHovererd = false;

	ProjectConfigurationWindow m_ProjectConfigurationWindow;
	ProjectSettings m_ProjectSettings;
	EditorConfigurationWindow m_EditorConfigurationWindow;
	UserSettings m_UserSettings;
	EditorReferenceManager m_ReferenceManager;
	WorldCreateConfig m_CurrentWorldCreateConfig;

	FileBrowser m_FileBrowser;
	PropertyRenderer m_PropertyRenderer;
	Entity* m_pSelectedEntity;

	bool m_bHasCopiedEntity = false;
	Document m_EntityCopyDocument;
	Value m_EntityCopyValue;

	EntityInspector m_EntityInspector;
	EntityHierarchy m_EntityHierarchy;
	ConsoleLog m_ConsoleLog;

	RenderContext* m_pRenderContext = nullptr;

	std::unordered_map<std::string, TextureReference*> m_Textures;
	std::unordered_map<std::string, EditorButton*> m_Buttons;

	bool m_bRenderMainMenuBar = true;
	bool m_bRenderGameInformation = false;
	bool m_bRenderWorldSettings = false;
	bool m_bRenderAboutVoxagine = false;
	bool m_bRenderCreateWorld = false;
	bool m_bRenderProjectSettings = false;
	bool m_bRenderEditorSettings = false;
	
	bool m_bRenderEditorWindows = true;
	bool m_bRenderEntityInspector = true;
	bool m_bRenderEntityHierarchy = true;
	bool m_bRenderConsoleLog = true;

	bool m_bRenderDebugLines = true;
	bool m_bInEditor = true;

	ImGuizmo::OPERATION m_bCurrentGizmoOperation = ImGuizmo::OPERATION::TRANSLATE;
	ImGuizmo::MODE m_bCurrentGizmoMode = ImGuizmo::MODE::WORLD;
	SnappingTool m_SnappingTool;

	bool m_bSelectedEntityTransforming;
	Matrix4 m_SelectedEntityTransformStart;

	std::string m_sWorldFilePath = "";
	std::string m_sWorldAutoSaveFilePath = "";
	std::string m_sWorldAutoSaveExtension = "_AutoSave";

	float m_fAutoSaveTimer = 0.f;
	bool m_bIsAutoSaving = false;

};