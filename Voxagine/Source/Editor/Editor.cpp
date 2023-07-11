#include "pch.h"
#include "Editor.h"

#include "Core/Application.h"
#include "Core/Platform/Platform.h"
#include "Core/Platform/Rendering/RenderContext.h"
#include <Core/Platform/Window/WindowContext.h>
#include "Core/Platform/Input/Temp/InputContextNew.h"
#include "Core/ECS/Systems/Physics/PhysicsSystem.h"

#include "Core/JsonSerializer.h"
#include "Core/ECS/WorldManager.h"
#include "Core/ECS/Systems/Rendering/RenderSystem.h"
#include "Editor/EditorWorld.h"

#include "Editor/UndoRedo/EditorEntityCommand.h"
#include "Editor/UndoRedo/CommandManager.h"

#include "Core/ECS/Entity.h"
#include "Core/ECS/Components/VoxRenderer.h"
#include "Core/ECS/Entities/Camera.h"
#include "Core/ECS/Components/Transform.h"
#include "Core/ECS/Components/SpriteRenderer.h"
#include "Core/ECS/Components/TextRenderer.h"
#include "Core/ECS/Entities/ViewPoint.h"

#include "Core/LoggingSystem/LoggingSystem.h"

#include "Editor/EditorWorld.h"
#include "Editor/UndoRedo/EditorTransformMatrixCommand.h"
#include "Editor/EditorCamera.h"
#include "EditorButton.h"

#include <External/imgui/imgui_internal.h>
#include "External/optick/optick.h"
#include "Core/Platform/Audio/AudioContext.h"

Editor::Editor()
	: m_EntityCopyValue(kObjectType)
{
}

Editor::~Editor()
{
}

void Editor::Initialize(Application * pTargetApplication)
{
	ViewPoint viewPoint(nullptr);
	m_CurrentWorldCreateConfig.SetWorldChunkSize(1);

	// Initialize Application pointer
	m_pApplication = pTargetApplication;

	m_PropertyRenderer.Initialize(this);
	m_EntityHierarchy.Initialize(this);
	m_EntityInspector.Initialize(this);
	m_ConsoleLog.Initialize(this);
	m_ReferenceManager.Initialize(this);

	m_ProjectConfigurationWindow.Initialize(this);
	m_ProjectConfigurationWindow.OnWindowClose += Event<>::Subscriber([this]() { m_bRenderProjectSettings = false; }, this);
	m_EditorConfigurationWindow.Initialize(this);
	m_EditorConfigurationWindow.OnWindowClose += Event<>::Subscriber([this]() { m_bRenderEditorSettings = false; }, this);

	m_pRenderContext = m_pApplication->GetPlatform().GetRenderContext();

	InitializeButtons();
	InitializeTextures();

	m_SnappingTool.SetSnappingValue(ImGuizmo::OPERATION::TRANSLATE, 0, 1);
	m_SnappingTool.SetSnappingValue(ImGuizmo::OPERATION::TRANSLATE, 1, 5);
	m_SnappingTool.SetSnappingValue(ImGuizmo::OPERATION::TRANSLATE, 2, 10);

	m_SnappingTool.SetSnappingValue(ImGuizmo::OPERATION::ROTATE, 0, 1);
	m_SnappingTool.SetSnappingValue(ImGuizmo::OPERATION::ROTATE, 1, 45);
	m_SnappingTool.SetSnappingValue(ImGuizmo::OPERATION::ROTATE, 2, 90);

	m_SnappingTool.SetSnappingValue(ImGuizmo::OPERATION::SCALE, 0, 1);
	m_SnappingTool.SetSnappingValue(ImGuizmo::OPERATION::SCALE, 1, 2);
	m_SnappingTool.SetSnappingValue(ImGuizmo::OPERATION::SCALE, 2, 3);

	// Register to the world loaded/created event
	GetApplication()->GetWorldManager().WorldLoaded += Event<World*>::Subscriber(std::bind(&Editor::OnWorldCreated, this, std::placeholders::_1), this);

	// Register to the world unloaded/popped event
	GetApplication()->GetWorldManager().WorldPopped += Event<World*>::Subscriber(std::bind(&Editor::OnWorldDestroyed, this, std::placeholders::_1), this);

	RenderContext* pRenderContext = GetApplication()->GetPlatform().GetRenderContext();
	pRenderContext->SizeChanged += Event<uint32_t, uint32_t, IVector2 >::Subscriber(std::bind(&Editor::OnResize, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3), this);

	InitializeProjectSettings();
	InitializeUserSettings();
	InitializeInput();

	CreateTemplateWorld(true);
}

void Editor::UnInitialize()
{
	m_ProjectSettings.SaveSettings();
	m_EntityHierarchy.UnInitialize();
	m_EntityInspector.UnInitialize();
	m_ConsoleLog.UnInitialize();
	m_ReferenceManager.UnInitialize();

	/* Delete GUI buttons */
	for (auto& it : m_Buttons)
	{
		delete it.second;
	}

	/* Delete referenced textures */
	for (auto& it : m_Textures)
	{
		it.second->Release();
	}

}

void Editor::WorldPreTick(World* pActiveWorld)
{
	pActiveWorld->PreTick();
}

void Editor::WorldTick(World* pActiveWorld, float fDeltaTime)
{
	pActiveWorld->Tick(fDeltaTime);
}

void Editor::WorldPostTick(World* pActiveWorld, float fDeltaTime)
{
	pActiveWorld->PostTick(fDeltaTime);

	UpdateAutoSaveWorld(fDeltaTime);
}

void Editor::WorldFixedTick(World* pActiveWorld, const GameTimer& fixedTimer)
{
	pActiveWorld->FixedTick(fixedTimer);
}

void Editor::WorldPostFixedTick(World* pActiveWorld, const GameTimer& fixedTimer)
{
	pActiveWorld->PostFixedTick(fixedTimer);
}

void Editor::WorldRender(World* pActiveWorld, const GameTimer& fixedTimer)
{
	pActiveWorld->Render(fixedTimer);
}

void Editor::Render(float fDeltaTime)
{
	OPTICK_CATEGORY("Editor render", Optick::Category::Rendering);
	OPTICK_EVENT();
	RenderGameWindow();
	RenderMainMenuBar();
	RenderWindowRenderInfo();
	RenderWindowWorldSettings();
	RenderWindowAboutVoxagine();
	RenderWindowCreateWorld();

	RenderConfigurationWindows(fDeltaTime);
	RenderEditorTools(fDeltaTime);
	RenderEntityTransformation();
}

void Editor::OnResize(uint32_t a_uiWidth, uint32_t a_uiHeight, IVector2 deltaResolution)
{
	m_EntityHierarchy.OnContextResize(a_uiWidth, a_uiHeight, deltaResolution);
	m_EntityInspector.OnContextResize(a_uiWidth, a_uiHeight, deltaResolution);
	m_ConsoleLog.OnContextResize(a_uiWidth, a_uiHeight, deltaResolution);
}

Application * Editor::GetApplication()
{
	return m_pApplication;
}

void Editor::SetSelectedEntity(Entity * pNewSelectedEntity)
{
	if (GetSelectedEntity() != pNewSelectedEntity)
	{
		ResetSelectedEntity();

		m_pSelectedEntity = pNewSelectedEntity;

		if (GetSelectedEntity() != nullptr)
			OutlineEntity(GetSelectedEntity());

		EntitySelected.operator()(GetSelectedEntity());
	}
}

void Editor::ResetSelectedEntity()
{
	if (HasSelectedEntity())
	{
		DeOutlineEntity(GetSelectedEntity());
		EntityDeSelected.operator()(GetSelectedEntity());

		m_pSelectedEntity = nullptr;
	}
}

bool Editor::HasSelectedEntity()
{
	return (GetSelectedEntity() != nullptr);
}

Entity * Editor::GetSelectedEntity()
{
	return m_pSelectedEntity;
}

bool Editor::IsModifyingSelectedEntityTransform() const
{
	return ImGuizmo::IsUsing();
}

bool Editor::IsMouseHoveringEditorWindows() const
{
	// TODO: Clean up and make better solution
	if (m_bGameFocus)
	{
		return false;
	}

	return (!m_bGameWindowFocused || !IsGameWindowHoverered() || m_bRenderAboutVoxagine || m_bRenderCreateWorld);
}

bool Editor::IsGameWindowHoverered() const
{
	return (m_bRenderEditorWindows) ? m_bGameWindowHovererd : false;
}

EditorModus Editor::GetEditorModus() const
{
	return m_EditorModus;
}

void Editor::SetTransformWorldModus(bool bWorldModus)
{
	m_bCurrentGizmoMode = (bWorldModus) ? ImGuizmo::WORLD : ImGuizmo::LOCAL;
}

bool Editor::IsTransformWorldModus() const
{
	return m_bCurrentGizmoMode;
}

PropertyRenderer & Editor::GetPropertyRenderer()
{
	return m_PropertyRenderer;
}

EditorWorld* Editor::GetEditorWorld()
{
	return m_pEditorWorld;
}

ProjectSettings & Editor::GetProjectSettings()
{
	return m_ProjectSettings;
}

UserSettings & Editor::GetUserSettings()
{
	return m_UserSettings;
}

bool Editor::GetCopyEntityData(Document*& entityCopyDocument, Value*& entityCopyValue)
{
	if (!m_bHasCopiedEntity)
		return false;

	entityCopyDocument = &m_EntityCopyDocument;
	entityCopyValue = &m_EntityCopyValue;

	return true;
}

void Editor::InitializeProjectSettings()
{
	// Load the project settings 
	m_ProjectSettings.Initialize(&GetApplication()->GetSerializer(), "ProjectSettings.vgps");
	
	LoadResources();
}

void Editor::LoadResources()
{
	// Search the content directory folder and gather all sub directories and files (any extension)
	FolderTree FolderTreeInformation;
	m_FileBrowser.GetFolderTreeInformation(m_ProjectSettings.GetContentFolderPath(), FolderTreeInformation);

	// Filter for vox models and import all those files
	std::vector<std::string> VoxModelFilePaths;
	FolderTreeInformation.GetFilteredFiles(VoxModelFilePaths, ".vox");
	m_ReferenceManager.ImportFiles(VoxModelFilePaths);

	// Filter for animated vox models and import all those files
	std::vector<std::string> AnimVoxModelFilePaths;
	FolderTreeInformation.GetFilteredFiles(AnimVoxModelFilePaths, ".anim.vox");
	m_ReferenceManager.ImportFiles(AnimVoxModelFilePaths);

	// Filter for oog sounds and import all those files
	std::vector<std::string> OogSoundFilePaths;
	FolderTreeInformation.GetFilteredFiles(OogSoundFilePaths, ".ogg");
	m_ReferenceManager.ImportFiles(OogSoundFilePaths);

	// Filter for png textures and import all those files
	std::vector<std::string> PngTextureFilePaths;
	FolderTreeInformation.GetFilteredFiles(PngTextureFilePaths, ".png");
	m_ReferenceManager.ImportFiles(PngTextureFilePaths);

	std::vector<std::string> WorldFilePaths;
	FolderTreeInformation.GetFilteredFiles(WorldFilePaths, ".wld");
	GetApplication()->GetWorldManager().SetWorldFiles(WorldFilePaths);

}

void Editor::InitializeUserSettings()
{
	// Load the user settings 
	m_UserSettings.Initialize(&GetApplication()->GetSerializer(), "UserSettings.vguser");
}

void Editor::InitializeInput()
{
	// TODO: Temp solution for storing input handles
	std::vector<uint64_t> m_ActionBindings;

	GetApplication()->GetPlatform().GetInputContext()->CreateBindingMap(EDITOR_INPUT_LAYER_NAME, true, BMT_GLOBAL);

	// Initialize bindings for the Game within Editor

	GetApplication()->GetPlatform().GetInputContext()->RegisterAction(DEFAULT_INPUT_MAP_NAME, "Focus_GameWindow", IKS_HELD, IK_LEFTSHIFT, {IK_F1}, BMT_GLOBAL);

	GetApplication()->GetPlatform().GetInputContext()->BindAction(DEFAULT_INPUT_MAP_NAME, "Focus_GameWindow", IKS_HELD, m_ActionBindings, BMT_GLOBAL, [&]() {
		if (!ImGui::IsAnyItemActive())
			FocusUnfocusGameWindow();
	});

	GetApplication()->GetPlatform().GetInputContext()->RegisterAction(DEFAULT_INPUT_MAP_NAME, "Possess_Eject_Player", IKS_HELD, IK_LEFTSHIFT, { IK_F2 }, BMT_GLOBAL);

	GetApplication()->GetPlatform().GetInputContext()->BindAction(DEFAULT_INPUT_MAP_NAME, "Possess_Eject_Player", IKS_HELD, m_ActionBindings, BMT_GLOBAL, [&]() {
		if (!ImGui::IsAnyItemActive())
			PossessEjectPlayer();
	});

	// Initialize bindings for Editor

	GetApplication()->GetPlatform().GetInputContext()->RegisterAction(EDITOR_INPUT_LAYER_NAME, "Focus_GameWindow", IKS_HELD, IK_LEFTSHIFT, { IK_F1 }, BMT_GLOBAL);

	GetApplication()->GetPlatform().GetInputContext()->BindAction(EDITOR_INPUT_LAYER_NAME, "Focus_GameWindow", IKS_HELD, m_ActionBindings, BMT_GLOBAL, [&]() {
		if (!ImGui::IsAnyItemActive())
			FocusUnfocusGameWindow();
	});

	GetApplication()->GetPlatform().GetInputContext()->RegisterAction(EDITOR_INPUT_LAYER_NAME, "Possess_Eject_Player", IKS_HELD, IK_LEFTSHIFT, { IK_F2 }, BMT_GLOBAL);

	GetApplication()->GetPlatform().GetInputContext()->BindAction(EDITOR_INPUT_LAYER_NAME, "Possess_Eject_Player", IKS_HELD, m_ActionBindings, BMT_GLOBAL, [&]() {
		if (!ImGui::IsAnyItemActive())
			PossessEjectPlayer();
	});

	GetApplication()->GetPlatform().GetInputContext()->RegisterAction(EDITOR_INPUT_LAYER_NAME, "Undo_Command", IKS_HELD, IK_LEFTCONTROL, { IK_Z }, BMT_GLOBAL);

	GetApplication()->GetPlatform().GetInputContext()->BindAction(EDITOR_INPUT_LAYER_NAME, "Undo_Command", IKS_HELD, m_ActionBindings, BMT_GLOBAL, [&]() {
		if (!ImGui::IsAnyItemActive())
			GetEditorWorld()->GetCommandManager().Undo();
	});

	GetApplication()->GetPlatform().GetInputContext()->RegisterAction(EDITOR_INPUT_LAYER_NAME, "Redo_Command", IKS_HELD, IK_LEFTCONTROL, { IK_Y }, BMT_GLOBAL);

	GetApplication()->GetPlatform().GetInputContext()->BindAction(EDITOR_INPUT_LAYER_NAME, "Redo_Command", IKS_HELD, m_ActionBindings, BMT_GLOBAL, [&]() {
		if (!ImGui::IsAnyItemActive())
			GetEditorWorld()->GetCommandManager().Redo();
	});

	GetApplication()->GetPlatform().GetInputContext()->RegisterAction(EDITOR_INPUT_LAYER_NAME, "Focus", IKS_PRESSED, { IK_F }, BMT_GLOBAL);
	GetApplication()->GetPlatform().GetInputContext()->BindAction(EDITOR_INPUT_LAYER_NAME, "Focus", IKS_PRESSED, m_ActionBindings, BMT_GLOBAL, [&]() {
		if (GetSelectedEntity() != nullptr && !ImGui::IsAnyItemActive())
		{
			if (rttr::type::get(*GetSelectedEntity()) == rttr::type::get<ViewPoint>())
			{
				static_cast<ViewPoint*>(GetSelectedEntity())->SetViewTarget(m_pEditorWorld->GetMainCamera());
			}
		}
	});

	GetApplication()->GetPlatform().GetInputContext()->RegisterAction(EDITOR_INPUT_LAYER_NAME, "Copy", IKS_HELD, IK_LEFTCONTROL, { IK_C }, BMT_GLOBAL);

	GetApplication()->GetPlatform().GetInputContext()->BindAction(EDITOR_INPUT_LAYER_NAME, "Copy", IKS_HELD, m_ActionBindings, BMT_GLOBAL, [&]() {
		if (HasSelectedEntity() && !ImGui::IsAnyItemActive())
		{
			m_EntityCopyValue = Value(kObjectType);
			m_EntityCopyDocument = Document();

			GetApplication()->GetSerializer().EntityToValue(GetSelectedEntity(), m_EntityCopyValue, m_EntityCopyDocument.GetAllocator());
			m_bHasCopiedEntity = true;
		}
	});

	GetApplication()->GetPlatform().GetInputContext()->RegisterAction(EDITOR_INPUT_LAYER_NAME, "Paste", IKS_HELD, IK_LEFTCONTROL, { IK_V }, BMT_GLOBAL);

	GetApplication()->GetPlatform().GetInputContext()->BindAction(EDITOR_INPUT_LAYER_NAME, "Paste", IKS_HELD, m_ActionBindings, BMT_GLOBAL, [&]() {
		if (m_bHasCopiedEntity & !ImGui::IsAnyItemActive())
		{
			EditorWorld* pActiveEditorWorld = dynamic_cast<EditorWorld*>(GetApplication()->GetWorldManager().GetTopWorld());
			EditorEntityCommand* pEntityDuplicateCommand = CreateDuplicateEntityCreationCommand(this, GetSelectedEntity());
			pActiveEditorWorld->GetCommandManager().AddCommand(pEntityDuplicateCommand);

			Entity* pNewCopiedEntity = pEntityDuplicateCommand->GetCreatedEntity();
			SetSelectedEntity(pNewCopiedEntity);
		}
	});

	GetApplication()->GetPlatform().GetInputContext()->RegisterAction(EDITOR_INPUT_LAYER_NAME, "Manipulate_Mode", IKS_HELD, IK_LEFTCONTROL, { IK_W }, BMT_GLOBAL);

	GetApplication()->GetPlatform().GetInputContext()->BindAction(EDITOR_INPUT_LAYER_NAME, "Manipulate_Mode", IKS_HELD, m_ActionBindings, BMT_GLOBAL, [&]() {
		if (!ImGui::IsAnyItemActive())
			m_bCurrentGizmoMode = (m_bCurrentGizmoMode == ImGuizmo::MODE::WORLD) ? ImGuizmo::MODE::LOCAL : ImGuizmo::MODE::WORLD;
	});

	GetApplication()->GetPlatform().GetInputContext()->RegisterAction(EDITOR_INPUT_LAYER_NAME, "Manipulate_Translation", IKS_HELD, IK_LEFTCONTROL, { IK_E }, BMT_GLOBAL);

	GetApplication()->GetPlatform().GetInputContext()->BindAction(EDITOR_INPUT_LAYER_NAME, "Manipulate_Translation", IKS_HELD, m_ActionBindings, BMT_GLOBAL, [&]() {
		if (!ImGui::IsAnyItemActive())
			m_bCurrentGizmoOperation = ImGuizmo::OPERATION::TRANSLATE;
	});

	GetApplication()->GetPlatform().GetInputContext()->RegisterAction(EDITOR_INPUT_LAYER_NAME, "Manipulate_Rotation", IKS_HELD, IK_LEFTCONTROL, { IK_R }, BMT_GLOBAL);

	GetApplication()->GetPlatform().GetInputContext()->BindAction(EDITOR_INPUT_LAYER_NAME, "Manipulate_Rotation", IKS_HELD, m_ActionBindings, BMT_GLOBAL, [&]() {
		if (!ImGui::IsAnyItemActive())
			m_bCurrentGizmoOperation = ImGuizmo::OPERATION::ROTATE;
	});

	GetApplication()->GetPlatform().GetInputContext()->RegisterAction(EDITOR_INPUT_LAYER_NAME, "Manipulate_Scalar", IKS_HELD, IK_LEFTCONTROL, { IK_T }, BMT_GLOBAL);

	GetApplication()->GetPlatform().GetInputContext()->BindAction(EDITOR_INPUT_LAYER_NAME, "Manipulate_Scalar", IKS_HELD, m_ActionBindings, BMT_GLOBAL, [&]() {
		if (!ImGui::IsAnyItemActive())
			m_bCurrentGizmoOperation = ImGuizmo::OPERATION::SCALE;
	});

	GetApplication()->GetPlatform().GetInputContext()->RegisterAction(EDITOR_INPUT_LAYER_NAME, "Manipulate_Operation_Index_1", IKS_HELD, IK_LEFTCONTROL, { IK_D1 }, BMT_GLOBAL);

	GetApplication()->GetPlatform().GetInputContext()->BindAction(EDITOR_INPUT_LAYER_NAME, "Manipulate_Operation_Index_1", IKS_HELD, m_ActionBindings, BMT_GLOBAL, [&]() {
		if (m_SnappingTool.GetUsingIndex(m_bCurrentGizmoOperation) != 0 && !ImGui::IsAnyItemActive())
			m_SnappingTool.SetUsingIndex(m_bCurrentGizmoOperation, 0);
	});

	GetApplication()->GetPlatform().GetInputContext()->RegisterAction(EDITOR_INPUT_LAYER_NAME, "Manipulate_Operation_Index_2", IKS_HELD, IK_LEFTCONTROL, { IK_D2 }, BMT_GLOBAL);

	GetApplication()->GetPlatform().GetInputContext()->BindAction(EDITOR_INPUT_LAYER_NAME, "Manipulate_Operation_Index_2", IKS_HELD, m_ActionBindings, BMT_GLOBAL, [&]() {
		if (m_SnappingTool.GetUsingIndex(m_bCurrentGizmoOperation) != 1 && !ImGui::IsAnyItemActive())
			m_SnappingTool.SetUsingIndex(m_bCurrentGizmoOperation, 1);
	});

	GetApplication()->GetPlatform().GetInputContext()->RegisterAction(EDITOR_INPUT_LAYER_NAME, "Manipulate_Operation_Index_3", IKS_HELD, IK_LEFTCONTROL, { IK_D3 }, BMT_GLOBAL);

	GetApplication()->GetPlatform().GetInputContext()->BindAction(EDITOR_INPUT_LAYER_NAME, "Manipulate_Operation_Index_3", IKS_HELD, m_ActionBindings, BMT_GLOBAL, [&]() {
		if (m_SnappingTool.GetUsingIndex(m_bCurrentGizmoOperation) != 2 && !ImGui::IsAnyItemActive())
			m_SnappingTool.SetUsingIndex(m_bCurrentGizmoOperation, 2);
	});


	// Quick saving bindings for world
	GetApplication()->GetPlatform().GetInputContext()->RegisterAction(EDITOR_INPUT_LAYER_NAME, "Quick_Save", IKS_HELD, IK_LEFTCONTROL, { IK_S }, BMT_GLOBAL);

	GetApplication()->GetPlatform().GetInputContext()->BindAction(EDITOR_INPUT_LAYER_NAME, "Quick_Save", IKS_HELD, m_ActionBindings, BMT_GLOBAL, [&]() {
		if (!ImGui::IsAnyItemActive())
			SaveEditorWorld(m_sWorldFilePath);
	});

	// Initialize bindings for Editor Camera
	GetApplication()->GetPlatform().GetInputContext()->RegisterAction(EDITOR_INPUT_LAYER_NAME, "Mouse_Action", IKS_RELEASED, IK_MOUSEBUTTONLEFT, BMT_GLOBAL);

	GetApplication()->GetPlatform().GetInputContext()->RegisterAxis(EDITOR_INPUT_LAYER_NAME, "EditorCamera_Forward", IK_W, 1.f, BMT_GLOBAL);

	GetApplication()->GetPlatform().GetInputContext()->RegisterAxis(EDITOR_INPUT_LAYER_NAME, "EditorCamera_Backward", IK_S, -1.f, BMT_GLOBAL);

	GetApplication()->GetPlatform().GetInputContext()->RegisterAxis(EDITOR_INPUT_LAYER_NAME, "EditorCamera_Left", IK_A, -1.f, BMT_GLOBAL);

	GetApplication()->GetPlatform().GetInputContext()->RegisterAxis(EDITOR_INPUT_LAYER_NAME, "EditorCamera_Right", IK_D, 1.f, BMT_GLOBAL);
}

void Editor::InitializeButtons()
{
	ImVec2 ButtonSize = ImVec2(26, 26);

	m_Buttons["play"] = new EditorButton(
		m_pApplication,
		"Engine/Assets/Sprites/Editor/play_default.png",
		"Engine/Assets/Sprites/Editor/play_hovered.png",
		"Engine/Assets/Sprites/Editor/play_clicked.png",
		ButtonSize
	);

	m_Buttons["pause"] = new EditorButton(
		m_pApplication,
		"Engine/Assets/Sprites/Editor/pause_default.png",
		"Engine/Assets/Sprites/Editor/pause_hovered.png",
		"Engine/Assets/Sprites/Editor/pause_clicked.png",
		ButtonSize
	);

	m_Buttons["stop"] = new EditorButton(
		m_pApplication,
		"Engine/Assets/Sprites/Editor/stop_default.png",
		"Engine/Assets/Sprites/Editor/stop_hovered.png",
		"Engine/Assets/Sprites/Editor/stop_clicked.png",
		ButtonSize
	);

	m_Buttons["restart"] = new EditorButton(
		m_pApplication,
		"Engine/Assets/Sprites/Editor/restart_default.png",
		"Engine/Assets/Sprites/Editor/restart_hovered.png",
		"Engine/Assets/Sprites/Editor/restart_clicked.png",
		ButtonSize
	);

	m_Buttons["world_inactive"] = new EditorButton(
		m_pApplication,
		"Engine/Assets/Sprites/Editor/world_inactive.png",
		"Engine/Assets/Sprites/Editor/world_hovered.png",
		"Engine/Assets/Sprites/Editor/world_active.png",
		ButtonSize
	);

	m_Buttons["world_active"] = new EditorButton(
		m_pApplication,
		"Engine/Assets/Sprites/Editor/world_active.png",
		"Engine/Assets/Sprites/Editor/world_hovered.png",
		"Engine/Assets/Sprites/Editor/world_inactive.png",
		ButtonSize
	);

	m_Buttons["translation_inactive"] = new EditorButton(
		m_pApplication,
		"Engine/Assets/Sprites/Editor/translation_inactive.png",
		"Engine/Assets/Sprites/Editor/translation_hovered.png",
		"Engine/Assets/Sprites/Editor/translation_active.png",
		ButtonSize
	);

	m_Buttons["translation_active"] = new EditorButton(
		m_pApplication,
		"Engine/Assets/Sprites/Editor/translation_active.png",
		"Engine/Assets/Sprites/Editor/translation_hovered.png",
		"Engine/Assets/Sprites/Editor/translation_inactive.png",
		ButtonSize
	);

	m_Buttons["rotation_inactive"] = new EditorButton(
		m_pApplication,
		"Engine/Assets/Sprites/Editor/rotation_inactive.png",
		"Engine/Assets/Sprites/Editor/rotation_hovered.png",
		"Engine/Assets/Sprites/Editor/rotation_active.png",
		ButtonSize
	);

	m_Buttons["rotation_active"] = new EditorButton(
		m_pApplication,
		"Engine/Assets/Sprites/Editor/rotation_active.png",
		"Engine/Assets/Sprites/Editor/rotation_hovered.png",
		"Engine/Assets/Sprites/Editor/rotation_inactive.png",
		ButtonSize
	);

	m_Buttons["scale_inactive"] = new EditorButton(
		m_pApplication,
		"Engine/Assets/Sprites/Editor/scale_inactive.png",
		"Engine/Assets/Sprites/Editor/scale_hovered.png",
		"Engine/Assets/Sprites/Editor/scale_active.png",
		ButtonSize
	);

	m_Buttons["scale_active"] = new EditorButton(
		m_pApplication,
		"Engine/Assets/Sprites/Editor/scale_active.png",
		"Engine/Assets/Sprites/Editor/scale_hovered.png",
		"Engine/Assets/Sprites/Editor/scale_inactive.png",
		ButtonSize
	);

	m_Buttons["search_console"] = new EditorButton(
		m_pApplication,
		"Engine/Assets/Sprites/Editor/search_default.png",
		"Engine/Assets/Sprites/Editor/search_hovered.png",
		"Engine/Assets/Sprites/Editor/search_clicked.png",
		ButtonSize
	);

	m_Buttons["erase_console"] = new EditorButton(
		m_pApplication,
		"Engine/Assets/Sprites/Editor/erase_default.png",
		"Engine/Assets/Sprites/Editor/erase_hovered.png",
		"Engine/Assets/Sprites/Editor/erase_clicked.png",
		ButtonSize
	);

	m_Buttons["search_hierarchy"] = new EditorButton(
		m_pApplication,
		"Engine/Assets/Sprites/Editor/search_default.png",
		"Engine/Assets/Sprites/Editor/search_hovered.png",
		"Engine/Assets/Sprites/Editor/search_clicked.png",
		ButtonSize
	);

	m_Buttons["erase_hierarchy"] = new EditorButton(
		m_pApplication,
		"Engine/Assets/Sprites/Editor/erase_default.png",
		"Engine/Assets/Sprites/Editor/erase_hovered.png",
		"Engine/Assets/Sprites/Editor/erase_clicked.png",
		ButtonSize
	);
}

void Editor::InitializeTextures()
{
	m_Textures["main_menu_bar_bg"] = m_pApplication->GetResourceManager().LoadTexture("Engine/Assets/Sprites/Editor/main_menu_bar_bg.png");
}

void Editor::SetWorldFilePath(std::string sWorldFilePath)
{
	size_t foundExtension = sWorldFilePath.rfind(".wld");
	if (foundExtension == std::string::npos)
		sWorldFilePath = sWorldFilePath + ".wld";

	std::string relativeFilePath;
	m_FileBrowser.AbsoluteToRelative(sWorldFilePath, relativeFilePath, m_ProjectSettings.GetContentFolderPath());

	std::string resolvedFilePath;
	resolvedFilePath = m_FileBrowser.ResolveFilePath(relativeFilePath);

	m_sWorldFilePath = resolvedFilePath;

	foundExtension = resolvedFilePath.rfind(".wld");
	m_sWorldAutoSaveFilePath = resolvedFilePath.insert(foundExtension, m_sWorldAutoSaveExtension);
}

void Editor::SaveEditorWorld(std::string sFilePath)
{
	if (GetEditorModus() == EditorModus::EM_EDITOR)
	{
		bool bCanSave = false;
		FH FileHandle = GetApplication()->GetFileSystem()->OpenFile(sFilePath.c_str(), FSOF_READ);

		if (FileHandle == INVALID_FH)
		{
			if (m_FileBrowser.SaveFile(sFilePath, GetApplication()->GetSettings().GetWorldFileExtension()))
			{
				if (sFilePath.substr(sFilePath.find_last_of(".") + 1) != GetApplication()->GetSettings().GetWorldFileExtension())
					sFilePath += "." + GetApplication()->GetSettings().GetWorldFileExtension();
				

				SetWorldFilePath(sFilePath);
				sFilePath = m_sWorldFilePath;
				bCanSave = true;
			}
		}
		else
		{
			GetApplication()->GetFileSystem()->CloseFile(FileHandle);
			bCanSave = true;
		}

		if (bCanSave)
		{
			EditorWorld * pWorld = static_cast<EditorWorld*>(GetApplication()->GetWorldManager().GetTopWorld());
			if (pWorld != nullptr)
			{
				pWorld->PrepareSerialization();
				m_pApplication->GetSerializer().SerializeWorldToFile(sFilePath, pWorld, [this](bool succeed) {
				
					if (succeed)
						m_pApplication->GetLoggingSystem().Log(LOGLEVEL_MESSAGE, "Editor", "Successfully saved the world!");
					else
						m_pApplication->GetLoggingSystem().Log(LOGLEVEL_ERROR, "Editor", "Failed to save the world!");
				});
				pWorld->UnPrepareSerialization();
			}
		}
	}
}

void Editor::UpdateAutoSaveWorld(float fDeltaTime)
{
	// Temp dirty code 
	if (m_bRenderProjectSettings || m_bRenderEditorSettings || m_bRenderAboutVoxagine || m_bRenderCreateWorld)
		return;

	if (GetEditorModus() == EditorModus::EM_EDITOR && m_UserSettings.IsAutoSaveEnabled())
	{
		m_fAutoSaveTimer -= fDeltaTime;

		if (m_fAutoSaveTimer <= 0.f)
		{
			AutoSaveWorld();
			m_fAutoSaveTimer = static_cast<float>(m_UserSettings.GetAutoSaveTime());
		}
	}
}

void Editor::AutoSaveWorld()
{
	if (GetEditorModus() == EditorModus::EM_EDITOR)
	{
		EditorWorld * pWorld = static_cast<EditorWorld*>(GetApplication()->GetWorldManager().GetTopWorld());
		if (pWorld != nullptr)
		{
			m_bIsAutoSaving = true;
			pWorld->PrepareSerialization();

			m_pApplication->GetSerializer().SerializeWorldToFile(m_sWorldAutoSaveFilePath, pWorld, [this](bool succeed) { 
				
				if (succeed)
					m_pApplication->GetLoggingSystem().Log(LOGLEVEL_MESSAGE, "Editor", "Auto-Save world successfully!");
				else
					m_pApplication->GetLoggingSystem().Log(LOGLEVEL_ERROR, "Editor", "Auto-Save world failed!");

				m_bIsAutoSaving = false; 
			
			});

			pWorld->UnPrepareSerialization();
		}
	}
}

void Editor::OnPlay(bool bAutoSave)
{
	GetApplication()->GetPlatform().GetRenderContext()->WaitForGPU();

	if (GetEditorModus() != EM_EDITOR)
		GetApplication()->GetPlatform().GetAudioContext()->ResumeBGM();

	if (GetEditorModus() == EM_EDITOR && HasEditorWorld())
	{
		// When auto save boolean is true, perform auto save logic
		if (bAutoSave)
		{
			m_fAutoSaveTimer = m_UserSettings.GetAutoSaveTime();
			AutoSaveWorld();
		}

		SetEditorModus(EM_PLAY);
		if (m_bRenderEditorWindows)
			m_bRenderEditorWindows = false;

		GetEditorWorld()->PrepareSerialization();
		GetEditorWorld()->PreTick();

		Document EditorWorldDocument;
		GetApplication()->GetSerializer().SerializeWorld(GetEditorWorld(), EditorWorldDocument);

		GetEditorWorld()->UnPrepareSerialization();
		GetEditorWorld()->PreTick();

		if (GetEditorWorld()->HasEditorCamera())
			GetEditorWorld()->GetEditorCamera()->SetEnabled(false);

		EditorWorld* NewPlayWorld = new EditorWorld(GetApplication(), this);
		GetApplication()->GetSerializer().DeserializeWorld(*NewPlayWorld, EditorWorldDocument);
		NewPlayWorld->SetWorldName(GetEditorWorld()->GetName());
		NewPlayWorld->PreLoad();

		GetApplication()->GetWorldManager().PushWorld(NewPlayWorld);

		ResetSelectedEntity();

		GetApplication()->GetPlatform().GetInputContext()->SetActiveBindingMap(DEFAULT_INPUT_MAP_NAME);
		m_bGameFocus = true;
		m_bPossessedPlayer = true;
	}
}

void Editor::OnPause()
{
	GetApplication()->GetPlatform().GetRenderContext()->WaitForGPU();
	GetApplication()->GetPlatform().GetAudioContext()->PauseBGM();

	if (!m_bRenderEditorWindows)
		m_bRenderEditorWindows = true;

	SetEditorModus(EM_PAUSE);

	GetApplication()->GetPlatform().GetInputContext()->SetActiveBindingMap(EDITOR_INPUT_LAYER_NAME);
	m_bGameFocus = false;
}

void Editor::OnEditor()
{
	GetApplication()->GetPlatform().GetRenderContext()->WaitForGPU();
	GetApplication()->GetPlatform().GetAudioContext()->StopBGM();

	GetApplication()->GetPlatform().GetRenderContext()->SetFadeValue(1.f);

	if (!m_bRenderEditorWindows)
		m_bRenderEditorWindows = true;

	SetEditorModus(EM_EDITOR);
	ResetSelectedEntity();

	World* CurrentWorld = GetApplication()->GetWorldManager().GetTopWorld();
	const std::vector<World*>& StackWorlds = GetApplication()->GetWorldManager().GetWorlds();

	for (std::vector<World*>::const_reverse_iterator it = StackWorlds.rbegin(); it != StackWorlds.rend();)
	{
		if (CurrentWorld != GetEditorWorld())
		{
			GetApplication()->GetWorldManager().PopWorld();
			++it;
			CurrentWorld = *it;
		}
		else
		{
			it = StackWorlds.rend();
		}
	}

	if (GetEditorWorld()->HasEditorCamera())
		GetEditorWorld()->GetEditorCamera()->SetEnabled(true);

	GetApplication()->GetPlatform().GetInputContext()->SetActiveBindingMap(EDITOR_INPUT_LAYER_NAME);
	m_bGameFocus = false;
	m_bPossessedPlayer = false;
}

void Editor::OnRestart()
{
	OnEditor();
	OnPlay(false);
}

void Editor::CreateTemplateWorld(bool bCreateProjectDefault, UVector2 chunkGridSize /*= UVector2(1, 1)*/)
{
	SetWorldFilePath(m_ProjectSettings.GetContentFolderPath() + "/" + "NewWorld.wld");
	EditorWorld* TemplateWorld = new EditorWorld(GetApplication(), this, chunkGridSize);

	if (bCreateProjectDefault)
	{
		const std::string outPath = m_ProjectSettings.GetDefaultMap();
		if (!outPath.empty())
		{
			Document EditorWorldDocument;

			if (!TemplateWorld->PreLoad(outPath))
			{
				GetApplication()->GetLoggingSystem().Log(LOGLEVEL_ERROR, "Editor", "Can't load world file since its version is not supported, file path: " + outPath);
			}
		}
	}

	GetApplication()->GetWorldManager().LoadWorld(TemplateWorld);
	GetApplication()->GetPlatform().GetRenderContext()->SetFadeValue(1.f);

	SetEditorModus(EM_EDITOR);
	SetEditorWorld(TemplateWorld);
}

void Editor::RenderGameWindow()
{
	ImGuiWindowFlags GameWindowFlags;
	GameWindowFlags = ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove;
	ImGui::PushStyleColor(ImGuiCol_WindowBg, ImVec4(0, 0, 0, 0));
	ImGui::PushStyleColor(ImGuiCol_Border, ImVec4(0, 0, 0, 0));

	UVector2 WindowSize = GetApplication()->GetPlatform().GetRenderContext()->GetRenderResolution();

	ImVec2 GameWindowPos = ImVec2(290.f, 27.f);
	ImVec2 GameWindowSize = ImVec2(WindowSize.x - GameWindowPos.x - 290.f, WindowSize.y - 250 - GameWindowPos.y);

	ImGui::SetNextWindowPos(GameWindowPos);
	ImGui::SetNextWindowSize(GameWindowSize);

	bool GameWindowVisIKle = true;

	ImGui::Begin("GAMEWINDOW", &GameWindowVisIKle, GameWindowFlags);

	m_bGameWindowFocused = ImGui::IsWindowFocused();
	m_bGameWindowHovererd = ImGui::IsWindowHovered();
	bool bInteractionGameWindow = false;

	if (m_bGameWindowHovererd)
	{
		for (unsigned int MouseButtonIndex = 0; MouseButtonIndex != 5; ++MouseButtonIndex)
		{
			if (ImGui::GetIO().MouseClicked[MouseButtonIndex])
			{
				ImGui::GetCurrentContext()->ActiveId = 0;
				m_bGameWindowFocused = true;
				ImGui::SetWindowFocus();

				bInteractionGameWindow = true;
			}
		}
	}

	if (bInteractionGameWindow && !m_bGameFocus && GetEditorModus() == EM_PLAY && m_bPossessedPlayer)
	{ 
		GetApplication()->GetPlatform().GetInputContext()->SetActiveBindingMap(DEFAULT_INPUT_MAP_NAME);
		SetRenderEditorWindows(!GetRenderEditorWindows());
		m_bGameFocus = true;
	}

	ImGui::End();

	ImGui::PopStyleColor();
	ImGui::PopStyleColor();
}

void Editor::RenderMainMenuBar()
{
	if (m_bRenderMainMenuBar)
	{
		ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(0.0f, 7.0f));
		ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);

		ImGui::BeginMainMenuBar();

		ImGui::Unindent(7.0f);
		ImGui::Image(*(ImTextureID*)m_Textures["main_menu_bar_bg"]->Descriptor, ImVec2(static_cast<float>(m_pRenderContext->GetRenderResolution().x), 26.0f));
		ImGui::Indent(7.0f);

		ImGui::SameLine(5.0f);

		if (ImGui::BeginMenu("File"))
		{
			if (ImGui::MenuItem("New World", NULL, false, GetEditorModus() == EditorModus::EM_EDITOR))
			{
				m_bRenderCreateWorld = true;
			}

			if (ImGui::MenuItem("Load World", nullptr, false, GetEditorModus() == EditorModus::EM_EDITOR))
			{
				std::string filePath;
				if (m_FileBrowser.OpenFile(filePath, GetApplication()->GetSettings().GetWorldFileExtension()))
				{
					if (filePath.substr(filePath.find_last_of('.') + 1) != GetApplication()->GetSettings().GetWorldFileExtension())
						filePath += "." + GetApplication()->GetSettings().GetWorldFileExtension();

					if (m_FileBrowser.AbsoluteToRelative(filePath, m_sWorldFilePath, m_ProjectSettings.GetContentFolderPath()))
					{
						EditorWorld* pLoadedWorld = new EditorWorld(GetApplication(), this);
						if (pLoadedWorld->PreLoad(m_sWorldFilePath))
						{
							GetApplication()->GetWorldManager().LoadWorld(pLoadedWorld);
							SetEditorWorld(pLoadedWorld);
							SetWorldFilePath(m_sWorldFilePath);

							GetApplication()->GetPlatform().GetRenderContext()->SetFadeValue(1.f);
						}
						else
						{
							m_pApplication->GetLoggingSystem().Log(LOGLEVEL_ERROR, "Editor", "Can't load world file since its version is not supported, file path: " + filePath);
							delete pLoadedWorld;
						}
					}
					else
					{
						m_pApplication->GetLoggingSystem().Log(LOGLEVEL_WARNING, "Editor", "Can't load file because it's not located in the Content folder");
					}
				}
			}

			ImGui::Separator();

			if (ImGui::MenuItem("Save World", NULL))
			{
				SaveEditorWorld(m_sWorldFilePath);
			}

			if (ImGui::MenuItem("Save World As...", NULL))
			{
				SaveEditorWorld();
			}

			if (ImGui::MenuItem("Export World as .vox", NULL))
			{
				std::string filePath;
				if (m_FileBrowser.SaveFile(filePath, "vox"))
				{
					if (filePath.substr(filePath.find_last_of(".") + 1) != "vox")
						filePath += ".vox";

					UVector3 worldSize;
					GetApplication()->GetWorldManager().GetTopWorld()->GetSystem<PhysicsSystem>()->GetVoxelGrid()->GetDimensions(worldSize.x, worldSize.y, worldSize.z);

					VoxModel::Save(GetApplication()->GetFileSystem(), filePath, worldSize, m_pRenderContext->GetVoxelData());
				}
			}

			ImGui::Separator();

			if (ImGui::MenuItem("Import .vox Model", NULL))
			{
				std::string filePath;
				if (m_FileBrowser.OpenFile(filePath, "vox"))
				{
					std::string outPath;
					if (m_FileBrowser.AbsoluteToRelative(filePath, outPath, m_ProjectSettings.GetContentFolderPath()))
					{
						m_ReferenceManager.ImportFile(outPath);
					}
					else
					{
						m_pApplication->GetLoggingSystem().Log(LOGLEVEL_WARNING, "Editor", "Can't load file because it's not located in the Content folder");
					}
				}
			}

			if (ImGui::MenuItem("Refresh Content Folder", NULL))
			{
				LoadResources();
			}

			ImGui::Separator();

			if (ImGui::MenuItem("Import .prefab", NULL))
			{
				std::string filePath;
				if (m_FileBrowser.OpenFile(filePath, GetApplication()->GetSettings().GetPrefabFileExtension()))
				{
					if (filePath.substr(filePath.find_last_of('.') + 1) != GetApplication()->GetSettings().GetPrefabFileExtension())
						filePath += "." + GetApplication()->GetSettings().GetPrefabFileExtension();

					std::string outPath;
					if (m_FileBrowser.AbsoluteToRelative(filePath, outPath, m_ProjectSettings.GetContentFolderPath()))
					{
						World* pActiveWorld = GetApplication()->GetWorldManager().GetTopWorld();
						EditorWorld* pActiveEditorWorld = dynamic_cast<EditorWorld*>(pActiveWorld);

						if (pActiveEditorWorld != nullptr)
						{
							Entity* pPrefab = m_pApplication->GetSerializer().DeserializeEntityFromFile(outPath, *pActiveWorld);
							pPrefab->GetTransform()->SetPosition(Vector3(0, 0, 0));
							SetSelectedEntity(pPrefab);
						}
						else m_pApplication->GetLoggingSystem().Log(LOGLEVEL_WARNING, "Editor", "Can't import prefab due to not being a EditorWorld!");
					}
				}
			}

			if (ImGui::MenuItem("Export Entity as .prefab", NULL, false, GetSelectedEntity()))
			{
				std::string filePath;
				if (m_FileBrowser.SaveFile(filePath, "." + GetApplication()->GetSettings().GetPrefabFileExtension()))
				{
					if (filePath.substr(filePath.find_last_of(".") + 1) != GetApplication()->GetSettings().GetPrefabFileExtension())
						filePath += "." + GetApplication()->GetSettings().GetPrefabFileExtension();

					m_pApplication->GetSerializer().SerializeEntityToFile(filePath, GetSelectedEntity());
				}
			}

			ImGui::EndMenu();
		}

		if (ImGui::BeginMenu("Edit"))
		{
			if (ImGui::MenuItem("Undo", NULL, false, GetEditorModus() == EM_EDITOR))
			{
				GetEditorWorld()->GetCommandManager().Undo();
			}

			if (ImGui::MenuItem("Redo", NULL, false, GetEditorModus() == EM_EDITOR))
			{
				GetEditorWorld()->GetCommandManager().Redo();
			}

			ImGui::EndMenu();
		}

		if (ImGui::BeginMenu("View"))
		{
			ImGui::MenuItem("Entity Inspector", NULL, &m_bRenderEntityInspector);
			ImGui::MenuItem("Entity Hierarchy", NULL, &m_bRenderEntityHierarchy);
			ImGui::MenuItem("Console Log", NULL, &m_bRenderConsoleLog);
			ImGui::MenuItem("Toggle Editor Windows", NULL, &m_bRenderEditorWindows);

			ImGui::Separator();

			if (ImGui::MenuItem("Draw Debug Lines", NULL, &m_bRenderDebugLines))
			{
				World* CurrentWorld = GetApplication()->GetWorldManager().GetTopWorld();

				CurrentWorld->GetRenderSystem()->EnableDebugLines(m_bRenderDebugLines);
				GetEditorWorld()->GetRenderSystem()->EnableDebugLines(m_bRenderDebugLines);
			}

			ImGui::EndMenu();
		}

		if (ImGui::BeginMenu("Settings"))
		{
			ImGui::MenuItem("Project Settings", NULL, &m_bRenderProjectSettings);
			ImGui::MenuItem("Editor Settings", NULL, &m_bRenderEditorSettings);
			ImGui::MenuItem("World Settings", NULL, &m_bRenderWorldSettings);
			ImGui::EndMenu();
		}

		if (ImGui::BeginMenu("Help"))
		{
			ImGui::MenuItem("Game Information", NULL, &m_bRenderGameInformation);
			if (ImGui::MenuItem("About Voxagine", NULL))
				m_bRenderAboutVoxagine = true;

			ImGui::EndMenu();
		}

		float WindowWidth = ImGui::GetWindowWidth();
		float ButtonSpacing = ImGui::GetStyle().ItemSpacing.x;

		int buttoncount = (GetEditorModus() == EM_EDITOR) ? 1 : 3;
		ImVec2 ButtonSize = ImVec2(26, 26);

		ImGui::SameLine((WindowWidth - (ButtonSize.x * buttoncount + (ButtonSpacing * (buttoncount + 1)))) / 2);

		ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0, 0, 0, 0));
		ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0, 0, 0, 0));
		ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0, 0, 0, 0));

		switch (GetEditorModus())
		{
		case (EM_EDITOR):
		{
			m_Buttons["play"]->OnClick([this] { OnPlay(true); });
		}
			break;
		case (EM_PLAY):
		{
			m_Buttons["pause"]->OnClick([this] { OnPause();  });
			m_Buttons["stop"]->OnClick([this] { OnEditor(); });
			m_Buttons["restart"]->OnClick([this] { OnRestart(); });
		}
			break;
		case (EM_PAUSE):
		{
			m_Buttons["play"]->OnClick([this] {
				SetEditorModus(EM_PLAY);

				GetApplication()->GetPlatform().GetRenderContext()->WaitForGPU();
				GetApplication()->GetPlatform().GetAudioContext()->ResumeBGM();
			});
			m_Buttons["stop"]->OnClick([this] { OnEditor(); });
			m_Buttons["restart"]->OnClick([this] { OnRestart(); });
		}
			break;
		};

		ImGui::SameLine(WindowWidth - (ButtonSize.x * 10.5f + (ButtonSpacing * 5.f)));

		for (size_t GizmoIt = 0; GizmoIt != 3; ++GizmoIt)
		{
			std::string GizmoIndexButtonName = std::to_string(GizmoIt) + "##SnappingToolIndex" + std::to_string(GizmoIt);

			if (m_SnappingTool.GetUsingIndex(m_bCurrentGizmoOperation) == GizmoIt)
			{
				ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.103f, 0.501f, 0.737f, 0.9f));
				ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.103f, 0.501f, 0.737f, 0.9f));
			}
			else
			{
				ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.145f, 0.156f, 0.180f, 0.9f));
				ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.145f, 0.156f, 0.180f, 0.9f));
			}

			if (ImGui::Button(GizmoIndexButtonName.c_str()))
			{
				if (m_SnappingTool.GetUsingIndex(m_bCurrentGizmoOperation) != GizmoIt)
					m_SnappingTool.SetUsingIndex(m_bCurrentGizmoOperation, GizmoIt);
			}
			ImGui::PopStyleColor();
			ImGui::PopStyleColor();

			ImGui::PushItemWidth(28);
			std::string GizmoIndexValueName = "##SnappingToolValue" + std::to_string(GizmoIt);
			int GizmoIndexValue = static_cast<int>(m_SnappingTool.GetSnappingValue(m_bCurrentGizmoOperation, GizmoIt));

			if (ImGui::InputScalarN(GizmoIndexValueName.c_str(), ImGuiDataType_S32, &GizmoIndexValue, 1))
			{
				if (GizmoIndexValue < 1)
					GizmoIndexValue = 1;

				m_SnappingTool.SetSnappingValue(m_bCurrentGizmoOperation, static_cast<size_t>(GizmoIndexValue), GizmoIt);
			}
			ImGui::PopItemWidth();
		}

		if (m_bCurrentGizmoMode == ImGuizmo::WORLD)
		{
			m_Buttons["world_active"]->OnClick([this] { m_bCurrentGizmoMode = ImGuizmo::LOCAL; });
		} 
		else
		{
			m_Buttons["world_inactive"]->OnClick([this] { m_bCurrentGizmoMode = ImGuizmo::WORLD; });		
		}

		if (m_bCurrentGizmoOperation == ImGuizmo::TRANSLATE)
		{
			m_Buttons["translation_active"]->OnClick([this] { m_bCurrentGizmoOperation = ImGuizmo::TRANSLATE; });
		}
		else
		{
			m_Buttons["translation_inactive"]->OnClick([this] { m_bCurrentGizmoOperation = ImGuizmo::TRANSLATE; });
		}

		if (m_bCurrentGizmoOperation == ImGuizmo::ROTATE)
		{
			m_Buttons["rotation_active"]->OnClick([this] { m_bCurrentGizmoOperation = ImGuizmo::ROTATE; });
		}
		else
		{
			m_Buttons["rotation_inactive"]->OnClick([this] { m_bCurrentGizmoOperation = ImGuizmo::ROTATE; });
		}

		if (m_bCurrentGizmoOperation == ImGuizmo::SCALE)
		{
			m_Buttons["scale_active"]->OnClick([this] { m_bCurrentGizmoOperation = ImGuizmo::SCALE; });
		}
		else
		{
			m_Buttons["scale_inactive"]->OnClick([this] { m_bCurrentGizmoOperation = ImGuizmo::SCALE; });
		}

		ImGui::PopStyleColor(3);

		ImGui::EndMainMenuBar();
		ImGui::PopStyleVar(2);
	}
}

void Editor::RenderEntityTransformation()
{
	if (!HasSelectedEntity())
		return;

	Matrix4 newMatrix, oldMatrix;

	/* Make sure the retrieved data is up-to-date */
	m_pSelectedEntity->GetTransform()->UpdateMatrix();

	oldMatrix = GetSelectedEntity()->GetTransform()->GetMatrix();
	newMatrix = oldMatrix;

	Camera* pCamera = GetApplication()->GetWorldManager().GetTopWorld()->GetMainCamera();

	Matrix4 viewMatrix = pCamera->GetView();
	Matrix4 projectionMatrix = pCamera->GetProjection();

	Matrix4 delta = Matrix4(1.f);

	// Screen space manipulation
	if (SpriteRenderer* pRenderer = GetSelectedEntity()->GetComponent<SpriteRenderer>())
	{
		if (pRenderer->IsScreenSpace())
		{
			Vector2 scale = pRenderer->GetScale();
			// float minScale = std::min(scale.x, scale.y);

			Vector2 screenAlignment = GetNormRenderAlignment(pRenderer->GetScreenAlignment());
			screenAlignment.y = 1.0f - screenAlignment.y;
			screenAlignment *= Vector2(m_pRenderContext->GetRenderResolution()) / scale;

			viewMatrix = glm::translate(Vector3(screenAlignment.x, screenAlignment.y, pCamera->GetFarPlane()));
			projectionMatrix = glm::ortho<float>(
				0.f, static_cast<float>(m_pRenderContext->GetRenderResolution().x) / scale.x,
				0.f, static_cast<float>(m_pRenderContext->GetRenderResolution().y) / scale.y,
				pCamera->GetNearPlane(), pCamera->GetFarPlane()
			);
		}
	}
	else if (TextRenderer* pRenderer = GetSelectedEntity()->GetComponent<TextRenderer>())
	{
		Vector2 scale = Vector2(1.f, 1.f);

		if (pRenderer->ScalesWithScreen())
		{
			Vector2 renderRes = m_pRenderContext->GetRenderResolution();
			scale = Vector2(renderRes.x / 1280.f, renderRes.y / 720.f);
		}

		Vector2 screenAlignment = GetNormRenderAlignment(pRenderer->GetScreenAlignment());
		screenAlignment.y = 1.0f - screenAlignment.y;
		screenAlignment *= Vector2(m_pRenderContext->GetRenderResolution()) / scale;

		viewMatrix = glm::translate(Vector3(screenAlignment.x, screenAlignment.y, pCamera->GetFarPlane()));
		projectionMatrix = glm::ortho<float>(
			0.f, static_cast<float>(m_pRenderContext->GetRenderResolution().x) / scale.x,
			0.f, static_cast<float>(m_pRenderContext->GetRenderResolution().y) / scale.y,
			pCamera->GetNearPlane(), pCamera->GetFarPlane()
			);
	}
	else if (TextRenderer* pRenderer = GetSelectedEntity()->GetComponent<TextRenderer>())
	{
		Vector2 scale = Vector2(1.f, 1.f);

		if (pRenderer->ScalesWithScreen())
		{
			Vector2 renderRes = m_pRenderContext->GetRenderResolution();
			scale = Vector2(renderRes.x / 1280.f, renderRes.y / 720.f);
		}

		Vector2 screenAlignment = GetNormRenderAlignment(pRenderer->GetScreenAlignment());
		screenAlignment.y = 1.0f - screenAlignment.y;
		screenAlignment *= Vector2(m_pRenderContext->GetRenderResolution()) / scale;

		viewMatrix = glm::translate(Vector3(screenAlignment.x, screenAlignment.y, pCamera->GetFarPlane()));
		projectionMatrix = glm::ortho<float>(
			0.f, static_cast<float>(m_pRenderContext->GetRenderResolution().x) / scale.x,
			0.f, static_cast<float>(m_pRenderContext->GetRenderResolution().y) / scale.y,
			pCamera->GetNearPlane(), pCamera->GetFarPlane()
		);
	}

	ImGuiIO& io = ImGui::GetIO();

	ImGuizmo::SetRect(0, 0, io.DisplaySize.x, io.DisplaySize.y);
	ImGuizmo::Manipulate(
		&viewMatrix[0][0],
		&projectionMatrix[0][0],
		m_bCurrentGizmoOperation,
		m_bCurrentGizmoMode,
		&newMatrix[0][0],
		&delta[0][0],
		m_SnappingTool.GetActiveSnappingValuePtr(m_bCurrentGizmoOperation)
	);

	/* Only changes when modified */
	if (delta != Matrix4(1.f)) 
	{
		if (GetEditorModus() != EM_PLAY && GetEditorWorld() != nullptr)
		{
			if (GetEditorWorld()->HasEditorCamera())
				GetEditorWorld()->GetEditorCamera()->LockThisFrame(true);
		}

		m_pSelectedEntity->GetTransform()->SetDirty(true);
	}

	if (m_bSelectedEntityTransforming != ImGuizmo::IsUsing())
	{
		if (ImGuizmo::IsUsing())
		{
			m_SelectedEntityTransformStart = oldMatrix;
		}
		else
		{
			EditorFunctionCommand* TransformMatrixCommand = CreateEditorTransformMatrixCommand(GetSelectedEntity()->GetTransform(), newMatrix, m_SelectedEntityTransformStart);
			GetEditorWorld()->GetCommandManager().AddCommand(TransformMatrixCommand);
		}

		m_bSelectedEntityTransforming = ImGuizmo::IsUsing();
	}

	if (ImGuizmo::IsUsing() && delta != Matrix4(1.f))
	{
		GetSelectedEntity()->GetTransform()->SetFromMatrix(newMatrix);
		VoxRenderer* pRenderer = GetSelectedEntity()->GetComponent<VoxRenderer>();
		if (pRenderer && pRenderer->IsChunkInstanceLoaded())
			pRenderer->RequestUpdate();
	}
}

void Editor::RenderConfigurationWindows(float fDeltaTime)
{
	if (m_bRenderProjectSettings)
		m_ProjectConfigurationWindow.Render(fDeltaTime);

	if (m_bRenderEditorSettings)
		m_EditorConfigurationWindow.Render(fDeltaTime);
}

void Editor::RenderEditorTools(float fDeltaTime)
{
	if (!m_bRenderEditorWindows)
		return;

	if (m_bRenderConsoleLog)
		m_ConsoleLog.Render(fDeltaTime);

	if (m_bRenderEntityInspector)
		m_EntityInspector.Render(fDeltaTime);

	if (m_bRenderEntityHierarchy)
		m_EntityHierarchy.Render(fDeltaTime);
}

void Editor::RenderWindowCreateWorld()
{
	if (m_bRenderCreateWorld)
	{
		if (!ImGui::IsPopupOpen("Create world"))
			ImGui::OpenPopup("Create world");

		ImVec2 PopUpModelSize = ImVec2(300, 120);
		UVector2 WindowSize = GetApplication()->GetPlatform().GetWindowContext()->GetSize();
		ImGui::SetNextWindowPos(ImVec2(static_cast<float>(WindowSize.x / 2.0f), static_cast<float>(WindowSize.y / 2.0f)), 0, ImVec2(0.5f, 0.5f));
		ImGui::SetNextWindowSize(PopUpModelSize);

		ImGui::BeginPopupModal("Create world", NULL, ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoSavedSettings);

		PropertyRenderer& pPropertyRenderer = GetPropertyRenderer();

		rttr::instance configInstance = m_CurrentWorldCreateConfig;
		rttr::type configType = configInstance.get_type();

		for (rttr::property propIter : configType.get_properties())
			pPropertyRenderer.Render(configInstance, propIter, "###" + configType.get_name().to_string() + propIter.get_name().to_string());

		if (ImGui::Button("Create"))
		{
			ImGui::CloseCurrentPopup();
			m_bRenderCreateWorld = false;

			uint32_t size = m_CurrentWorldCreateConfig.GetWorldChunkSize();
			CreateTemplateWorld(false, UVector2(size, size));
			m_CurrentWorldCreateConfig = WorldCreateConfig();
		}

		ImGui::SameLine();

		if (ImGui::Button("Close"))
		{
			ImGui::CloseCurrentPopup();
			m_bRenderCreateWorld = false;
			m_CurrentWorldCreateConfig = WorldCreateConfig();
		}

		ImGui::EndPopup();
	}
}

void Editor::RenderWindowRenderInfo()
{
	if (m_bRenderGameInformation)
	{
		if (ImGui::Begin("Game Information", &m_bRenderGameInformation))
		{
			std::string renderingType;

			switch (GetApplication()->GetSettings().GetRenderAPIType())
			{
			case RA_DIRECTX12:
				renderingType = "DirectX 12";
				break;
			case RA_OPENGL:
				renderingType = "OpenGL";
				break;
			case RA_OPENGLES:
				renderingType = "OpenGLES";
				break;
			case RA_VULKAN:
				renderingType = "Vulkan";
				break;
			default:
				renderingType = "Unknown";
				break;
			}

			ImGui::Text("Active Rendering Context:\n%s\n\n", renderingType.c_str());

			ImGui::Text("Active GPU device:\n%ls\n\n", GetApplication()->GetSettings().GetGPUName().c_str());

			RenderContext* pRenderContext = GetApplication()->GetPlatform().GetRenderContext();
			ImGui::Text("Current render resolution:\n%d x %d\n\n", pRenderContext->GetRenderResolution().x, pRenderContext->GetRenderResolution().y);

			uint32_t uiCPUTime = GetApplication()->GetTimer().GetFramesPerSecond();
			uint32_t uiGPUTime = pRenderContext->GetFPS();

			ImGui::Text("CPU: %.3f ms/frame (%d FPS)", 1000.0f / uiCPUTime, uiCPUTime);
			ImGui::Text("GPU: %.3f ms/frame (%d FPS)", 1000.0f / uiGPUTime, uiGPUTime);

			PhysicsSystem* pPhysics = GetApplication()->GetWorldManager().GetTopWorld()->GetSystem<PhysicsSystem>();
			ImGui::Text("Simulating particles: %i", pPhysics->m_uiActiveParticleCount);

			ImGui::End();
		}
	}
}

void Editor::RenderWindowWorldSettings()
{
	if (m_bRenderWorldSettings)
	{
		if (ImGui::Begin("World Settings", &m_bRenderWorldSettings))
		{
			std::string texturePath = m_pEditorWorld->GetGroundTexturePath();

			if (texturePath.empty())
				texturePath = "No file selected";

			ImGui::Text(texturePath.c_str());

			if (ImGui::Button(std::string("Set Ground Texture").c_str()))
			{
				if (m_FileBrowser.OpenFile(texturePath, "png"))
				{
					std::string outPath;
					if (m_FileBrowser.AbsoluteToRelative(texturePath, outPath, m_ProjectSettings.GetContentFolderPath()))
					{
						m_pEditorWorld->SetGroundTexturePath(outPath);
					}
					else
					{
						m_pApplication->GetLoggingSystem().Log(LOGLEVEL_WARNING, "Editor", "Can't load file because it's not located in the Content folder");
					}
				}
			}
		}

		ImGui::End();
	}
}

void Editor::RenderWindowAboutVoxagine()
{
	if (m_bRenderAboutVoxagine)
	{
		if (!ImGui::IsPopupOpen("About Voxagine"))
			ImGui::OpenPopup("About Voxagine");

		ImVec2 PopUpModelSize = ImVec2(320, 480);
		UVector2 WindowSize = GetApplication()->GetPlatform().GetWindowContext()->GetSize();
		ImGui::SetNextWindowPos(ImVec2(static_cast<float>(WindowSize.x / 2.0f), static_cast<float>(WindowSize.y / 2.0f)), 0, ImVec2(0.5f, 0.5f));

		ImGui::BeginPopupModal("About Voxagine", NULL, ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoSavedSettings);
		
		ImGui::Text("This application is strictly private, confidential and personal to");
		ImGui::Text("its recipients and should not be copied, distributed or reproduced");
		ImGui::Text("in whole or in part, nor passed to any third party.\n\n");

		ImGui::Text("Credits:\n\n");
		ImGui::Text("Arthur Kuylaars");
		ImGui::Text("Valencio Hoffman");
		ImGui::Text("Joey Jacobs");
		ImGui::Text("Erico Wiggers");
		ImGui::Text("Menno Markus");
		ImGui::Text("Pelle Ladegaard");
		ImGui::Text("Demir Ametov");
		ImGui::Text("Niels van Steeg");
		ImGui::Text("Robin Schulenberg");
		ImGui::Text("Wouter Grutter\n\n");

		ImGui::Text("Editor Controls: \n\n");

		ImGui::Text("Alt + Enter: Toggle full screen\n");
		ImGui::Text("F12: Toggle editor windows\n");

		ImGui::Text("Left Mouse Drag: Move camera\n");
		ImGui::Text("Right Mouse Drag: Rotate camera\n");
		ImGui::Text("Middle Mouse Scroll: Zoom in and out\n\n");

		ImGui::Text("WASD keys + Right Mouse Drag: Move camera + Rotate camera\n\n");

		ImGui::Text("Key E: Translation mode\n");
		ImGui::Text("Key R: Rotation mode\n");
		ImGui::Text("Key T: Scale mode\n");
		ImGui::Text("Key Y: Toggle world/local transformation mode\n\n");

		ImVec2 ButtonSize = ImVec2(96, 16);

		if (ImGui::Button("Got it!", ButtonSize))
		{
			ImGui::CloseCurrentPopup();
			m_bRenderAboutVoxagine = false;
		}

		ImGui::EndPopup();
	}
}

void Editor::OutlineEntity(Entity * pTargetEntity)
{
	SetOutlineEntity(pTargetEntity, true);
}

void Editor::DeOutlineEntity(Entity * pTargetEntity)
{
	SetOutlineEntity(pTargetEntity, false);
}

void Editor::SetOutlineEntity(Entity * pTargetEntity, bool bOutline)
{
	if (pTargetEntity != nullptr)
	{
		VoxRenderer* EntityVoxRenderer = EntityHasComponentVoxRenderer(pTargetEntity);

		if (EntityVoxRenderer != nullptr)
			EntityVoxRenderer->SetState((bOutline) ? RenderState::RS_SELECTION_LINES : RenderState::RS_DEFAULT);
	}
}

void Editor::SetRenderEditorWindows(bool bRenderEnabled)
{
	m_bRenderEditorWindows = bRenderEnabled;
}

bool Editor::GetRenderEditorWindows() const
{
	return m_bRenderEditorWindows;
}

EditorButton* Editor::GetButton(const std::string& buttonName)
{

	auto find = m_Buttons.find(buttonName);

	if (find != m_Buttons.end())
	{
		return find->second;
	}

	return nullptr;
}

VoxRenderer * Editor::EntityHasComponentVoxRenderer(Entity * pTargetEntity)
{
	return pTargetEntity->GetComponent<VoxRenderer>();
}

void Editor::SetEditorWorld(EditorWorld * pEditorWorld)
{
	m_pEditorWorld = pEditorWorld;
}

bool Editor::HasEditorWorld()
{
	return (GetEditorWorld() != nullptr);
}

void Editor::SetEditorModus(EditorModus NewEditorModus)
{
	m_EditorModus = NewEditorModus;
}

void Editor::OnWorldCreated(World * pCreatedWorld)
{
	pCreatedWorld->EntityRemoved += Event<Entity*>::Subscriber(std::bind(&Editor::OnEntityDestroyed, this, std::placeholders::_1), this);
}

void Editor::OnWorldDestroyed(World * pDestroyedWorld)
{
	pDestroyedWorld->EntityAdded -= this;
	pDestroyedWorld->EntityRemoved -= this;
	ResetSelectedEntity();
}

void Editor::OnEntityCreated(Entity * pCreatedEntity)
{
	if (pCreatedEntity->GetParent() != nullptr && HasSelectedEntity() && EntityHasComponentVoxRenderer(pCreatedEntity))
	{
		Entity* CurrentParent = pCreatedEntity->GetParent();

		while (CurrentParent != nullptr)
		{
			if (CurrentParent == GetSelectedEntity())
			{
				DeOutlineEntity(pCreatedEntity);
				return;
			}

			CurrentParent = CurrentParent->GetParent();
		}
	}
}

void Editor::OnEntityDestroyed(Entity * pDestroyedEntity)
{
	if (GetSelectedEntity() == pDestroyedEntity)
		ResetSelectedEntity();
}

void Editor::FocusUnfocusGameWindow()
{
	if (GetEditorModus() != EM_EDITOR)
	{
		if (m_bGameFocus)
		{
			SetRenderEditorWindows(true);
			GetApplication()->GetPlatform().GetInputContext()->SetActiveBindingMap(EDITOR_INPUT_LAYER_NAME);
			m_bGameFocus = false;
		}
		else
		{
			SetRenderEditorWindows(false);
			GetApplication()->GetPlatform().GetInputContext()->SetActiveBindingMap(DEFAULT_INPUT_MAP_NAME);
			m_bGameFocus = true;
		}
	}
}

void Editor::PossessEjectPlayer()
{
	if (GetEditorModus() != EM_EDITOR)
	{
		EditorWorld* pCurrentWorld = (EditorWorld*)GetApplication()->GetWorldManager().GetTopWorld();
		if (pCurrentWorld == nullptr)
		{
			m_pApplication->GetLoggingSystem().Log(LOGLEVEL_WARNING, "Editor", "Possess or UnPossess player due to not a valid EditorWorld on stack!");
			return;
		}

		if (m_bPossessedPlayer)
		{
			GetApplication()->GetPlatform().GetInputContext()->SetActiveBindingMap(EDITOR_INPUT_LAYER_NAME);
			pCurrentWorld->GetEditorCamera()->GetTransform()->SetFromMatrix(pCurrentWorld->GetMainCamera()->GetTransform()->GetMatrix());
			pCurrentWorld->GetEditorCamera()->SetEnabled(true);
			pCurrentWorld->SetMainCamera(pCurrentWorld->GetEditorCamera());
			m_bPossessedPlayer = false;
		}
		else
		{
			GetApplication()->GetPlatform().GetInputContext()->SetActiveBindingMap(DEFAULT_INPUT_MAP_NAME);
			pCurrentWorld->GetEditorCamera()->SetEnabled(false);
			pCurrentWorld->SetMainCamera(pCurrentWorld->GetPlayerCamera());
			pCurrentWorld->GetMainCamera()->ForceUpdate();
			m_bPossessedPlayer = true;
		}
	}
}