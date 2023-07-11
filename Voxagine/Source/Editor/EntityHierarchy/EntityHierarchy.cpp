#include "pch.h"
#include "EntityHierarchy.h"

#include "Editor/Editor.h"
#include "Core/ECS/Entity.h"
#include "Core/Application.h"
#include "Core/ECS/WorldManager.h"
#include "Editor/EditorWorld.h"
#include "Editor/EditorButton.h"
#include <External/imgui/imgui_stl.h>

#include "Editor/UndoRedo/CommandManager.h"
#include "Editor/UndoRedo/EditorEntityCommand.h"
#include "Editor/UndoRedo/EditorEntityChildCommand.h"

#include <algorithm>

EntityHierarchy::EntityHierarchy()
{
}

EntityHierarchy::~EntityHierarchy()
{
}

void EntityHierarchy::Initialize(Editor* pTargetEditor)
{
	Window::Initialize(pTargetEditor);

	SetName("Entity Hierarchy");

	SetPosition(ImVec2(0, 27.0f));
	SetSize(ImVec2(290.0f, 720.0f - 26.f));

	SetWindowFlag(ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoMove);
}

void EntityHierarchy::UnInitialize()
{
	Window::UnInitialize();
}

void EntityHierarchy::OnContextResize(uint32_t a_uiWidth, uint32_t a_uiHeight, IVector2 deltaResolution)
{
	SetSize(ImVec2(GetSize().x, static_cast<float>(a_uiHeight) - 26.f));
	SetPosition(ImVec2(0.0f, 27.0f));

	ImGui::SetWindowPos(GetName().data(), GetPosition());
	ImGui::SetWindowSize(GetName().data(), GetSize());
}

void EntityHierarchy::Tick(float fDeltaTime)
{

}

void EntityHierarchy::OnPreRender(float fDeltaTime)
{
	ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(17.0f, 5.0f));
}

void EntityHierarchy::OnRender(float fDeltaTime)
{
	ImGui::PopStyleVar();

	RenderCustomToolBar();

	RenderPopUpWindow();

	RenderEntityHierarchy();
}

void EntityHierarchy::OnSearch()
{
	std::transform(m_TempSearchString.begin(), m_TempSearchString.end(), m_TempSearchString.begin(), tolower);

	m_SearchString.assign(m_TempSearchString);
	++m_UniqueSearchID;
}

void EntityHierarchy::RenderCustomToolBar()
{
	// Render function for custom toolbar
	ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(5.0f, 7.0f));

	// Input text box for the search functionality
	ImGui::PushItemWidth(205);
	if (ImGui::InputText("###EntityHierarchySearch", &m_TempSearchString, ImGuiInputTextFlags_EnterReturnsTrue))
	{
		OnSearch();
	}

	ImGui::PopStyleVar();

	// Button for start search function
	ImGui::SameLine();

	ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0, 0, 0, 0));
	ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0, 0, 0, 0));
	ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0, 0, 0, 0));

	GetEditor()->GetButton("search_hierarchy")->OnClick([this] { OnSearch(); });

	// Button for erasing the search function
	ImGui::SameLine();

	GetEditor()->GetButton("erase_hierarchy")->OnClick([this] {
		m_TempSearchString.clear();
		m_SearchString.clear();
	});

	ImGui::PopStyleColor(3);
}

void EntityHierarchy::RenderPopUpWindow()
{
	bool RenderCreateCustomEntity = ImGui::IsPopupOpen("CreateCustomEntity");

	const ImVec2 SelectableSize = ImGui::GetItemRectSize();
	ImGui::SetNextWindowSize(ImVec2(SelectableSize.x * 8.0f, 0.0f));
	if (ImGui::BeginPopupContextWindow("Pop Up Window", 1, false))
	{
		if (ImGui::Selectable("Create Plain Entity"))
		{
			rttr::type NewPlanEntity = rttr::type::get<Entity>();
			EditorWorld* pActiveEditorWorld = dynamic_cast<EditorWorld*>(GetEditor()->GetApplication()->GetWorldManager().GetTopWorld());
			EditorEntityCommand* pEditorEntityCommand = CreateEntityCreationCommand(GetEditor(), NewPlanEntity, pActiveEditorWorld, nullptr);
			pActiveEditorWorld->GetCommandManager().AddCommand(pEditorEntityCommand);

			GetEditor()->SetSelectedEntity(pEditorEntityCommand->GetCreatedEntity());
		}

		if (ImGui::Selectable("Create Custom Entity"))
		{
			if (!RenderCreateCustomEntity)
			{
				RenderCreateCustomEntity = true;
				m_AddCustomEntitySearchString.clear();
			}
		}

		ImGui::EndPopup();
	}

	if (RenderCreateCustomEntity)
	{

		ImGui::SetNextWindowSize(ImVec2(0.0f, 17.f * 10.f));

		if (!ImGui::IsPopupOpen("CreateCustomEntity"))
			ImGui::OpenPopup("CreateCustomEntity");

		if (ImGui::BeginPopup("CreateCustomEntity"))
		{
			if (ImGui::IsRootWindowOrAnyChildFocused() && !ImGui::IsAnyItemActive() && !ImGui::IsMouseClicked(0))
				ImGui::SetKeyboardFocusHere(0);

			ImGui::InputText("###CustomEntitySearch", (char*)m_AddCustomEntitySearchString.c_str(), m_AddCustomEntitySearchString.capacity());
			const std::string SearchString = m_AddCustomEntitySearchString.c_str();

			ImGui::BeginChild("CustomEntitySelection");

			auto EntityTypes = rttr::type::get<Entity>().get_derived_classes();
			std::vector<std::string> EntityTypeNames;

			for (auto EntityTypeIt = EntityTypes.begin(); EntityTypeIt != EntityTypes.end(); ++EntityTypeIt)
			{
				if (EntityTypeIt->get_constructor({ rttr::type::get<World*>() }).is_valid())
				{
					EntityTypeNames.push_back(EntityTypeIt->get_name().to_string());
				}
			}

			std::sort(EntityTypeNames.begin(), EntityTypeNames.end());
			std::string SearchStringUpperCase = SearchString;
			for (char& SSIt : SearchStringUpperCase)
				SSIt = static_cast<char>(toupper(static_cast<int>(SSIt)));

			for (std::string& EntityTypeNameIt : EntityTypeNames)
			{
				std::string EntityTypeNameItUpperCase = EntityTypeNameIt;
				for (char& ETNIt : EntityTypeNameItUpperCase)
					ETNIt = static_cast<char>(toupper(static_cast<int>(ETNIt)));

				if (EntityTypeNameItUpperCase.find(SearchStringUpperCase) != std::string::npos || SearchString.empty())
				{
					if (ImGui::Selectable(std::string(EntityTypeNameIt + "###SearchAddCustomEntity" + EntityTypeNameIt).data()))
					{
						rttr::type NewCustomEntityType = rttr::type::get_by_name(EntityTypeNameIt);

						if (NewCustomEntityType.is_valid())
						{
							EditorWorld* pActiveEditorWorld = dynamic_cast<EditorWorld*>(GetEditor()->GetApplication()->GetWorldManager().GetTopWorld());
							EditorEntityCommand* pEditorEntityCommand = CreateEntityCreationCommand(GetEditor(), NewCustomEntityType, pActiveEditorWorld, nullptr);
							pActiveEditorWorld->GetCommandManager().AddCommand(pEditorEntityCommand);
							GetEditor()->SetSelectedEntity(pEditorEntityCommand->GetCreatedEntity());

							ImGui::CloseCurrentPopup();
						}
					}
				}
			}
			ImGui::EndChild();
			ImGui::EndPopup();
		}
	}
}

void EntityHierarchy::RenderPopUpEntityWindow(Entity* pEntity)
{
	std::string PopupName = "EntityProperties_" + std::to_string(pEntity->GetId());

	if (ImGui::BeginPopupContextItem(PopupName.c_str(), 1))
	{
		if (ImGui::Selectable("Destroy Entity"))
		{
			EditorWorld* pActiveEditorWorld = dynamic_cast<EditorWorld*>(GetEditor()->GetApplication()->GetWorldManager().GetTopWorld());
			EditorEntityCommand* pEditorEntityCommand = CreateEntityDestroyCommand(GetEditor(), pEntity);
			pActiveEditorWorld->GetCommandManager().AddCommand(pEditorEntityCommand);
		};

		ImGui::EndPopup();
	}
}

void EntityHierarchy::RenderEntityHierarchy()
{
	const std::vector<Entity*>& Entities = GetEditor()->GetApplication()->GetWorldManager().GetTopWorld()->GetEntities();

	Entity* SelectedEntity = GetEditor()->GetSelectedEntity();
	std::string WorldName = "World";

	if (ImGui::TreeNodeEx((char*)WorldName.c_str(), ImGuiTreeNodeFlags_DefaultOpen | ImGuiTreeNodeFlags_Leaf))
	{
		if (ImGui::BeginDragDropTarget())
		{
			if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("Entity_Drag-And-Drop"))
			{
				const auto payload_n = static_cast<Entity**>(payload->Data);

				if (payload_n != nullptr)
				{
					(*payload_n)->SetParent(nullptr);
				}
			}

			ImGui::EndDragDropTarget();
		}
	}

	for (Entity* it : Entities)
	{
		if (!it->GetParent())
		{
			if (m_SearchString.empty())
			{
				RenderEntity(it, SelectedEntity);
			}
			else
			{
				EntitySearchNode CurrentEntitySearchNode = EntitySearchNode(it, nullptr);
				EntityNeedsToBeRendererd(it, CurrentEntitySearchNode, m_SearchString);

				RenderEntitySearch(CurrentEntitySearchNode, SelectedEntity);
			}
		}
	}

	ImGui::TreePop();
}

void EntityHierarchy::RenderEntity(Entity * pTargetEntity, Entity* pSelectedEntity)
{
	ImGuiTreeNodeFlags TreeNodeFlags = ImGuiTreeNodeFlags_OpenOnDoubleClick | ImGuiTreeNodeFlags_OpenOnArrow;

	if (pTargetEntity->GetChildren().empty())
		TreeNodeFlags = TreeNodeFlags | ImGuiTreeNodeFlags_Leaf;

	if (pSelectedEntity == pTargetEntity)
		TreeNodeFlags = TreeNodeFlags | ImGuiTreeNodeFlags_Selected;

	const auto& io = ImGui::GetIO();

	if (ImGui::TreeNodeEx(GenerateTreeNodeLabel(pTargetEntity).c_str(), TreeNodeFlags))
	{
		// if we are dragging the entity around
		if (io.MouseDown[0] && ImGui::IsMouseDragging(0))
		{
			m_pSelectedEntity = nullptr;
			bClickOnEntity = false;
		}

		if (bClickOnEntity && io.MouseReleased[0])
		{
			GetEditor()->SetSelectedEntity(m_pSelectedEntity);
			bClickOnEntity = false;
		}

		if (IsCurrentTreeNodeClicked())
		{
			m_pSelectedEntity = pTargetEntity;
			bClickOnEntity = true;
		}

		RenderPopUpEntityWindow(pTargetEntity);

		ProcessEntityDragAndDrop(pTargetEntity);

		RenderChildEntities(pTargetEntity, pSelectedEntity);

		ImGui::TreePop();
	}
	else
	{
		// if we are dragging the entity around
		if (io.MouseDown[0] && ImGui::IsMouseDragging(0))
		{
			m_pSelectedEntity = nullptr;
			bClickOnEntity = false;
		}

		if (bClickOnEntity && io.MouseReleased[0])
		{
			GetEditor()->SetSelectedEntity(m_pSelectedEntity);
			bClickOnEntity = false;
		}

		if (IsCurrentTreeNodeClicked())
		{
			m_pSelectedEntity = pTargetEntity;
			bClickOnEntity = true;
		}

		RenderPopUpEntityWindow(pTargetEntity);

		ProcessEntityDragAndDrop(pTargetEntity);
	}
}

void EntityHierarchy::RenderChildEntities(Entity * pTargetEntity, Entity* pSelectedEntity)
{
	const std::vector<Entity*>& ChildEntities = pTargetEntity->GetChildren();

	for (Entity* it : ChildEntities)
		RenderEntity(it, pSelectedEntity);
}

void EntityHierarchy::ProcessEntityDragAndDrop(Entity * pTargetEntity)
{
	if (ImGui::BeginDragDropSource(ImGuiDragDropFlags_SourceAllowNullID)) {
		ImGui::SetDragDropPayload("Entity_Drag-And-Drop", &pTargetEntity, sizeof(Entity*), ImGuiCond_Once);

		ImGui::Text((char*)pTargetEntity->GetName().c_str());

		ImGui::EndDragDropSource();
	}

	if (ImGui::BeginDragDropTarget())
	{
		if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("Entity_Drag-And-Drop"))
		{
			const auto payload_n = static_cast<Entity**>(payload->Data);

			if (payload_n != nullptr)
			{
				if ((*payload_n)->GetParent() == pTargetEntity)
				{
					if (pTargetEntity->GetParent())
					{
						EditorWorld* pActiveEditorWorld = dynamic_cast<EditorWorld*>(GetEditor()->GetApplication()->GetWorldManager().GetTopWorld());
						EditorFunctionCommand* EntityAttachCommand = CreateEntityAttachToParentCommand((*payload_n), pTargetEntity->GetParent());
						pActiveEditorWorld->GetCommandManager().AddCommand(EntityAttachCommand);
					}
					else
					{
						EditorWorld* pActiveEditorWorld = dynamic_cast<EditorWorld*>(GetEditor()->GetApplication()->GetWorldManager().GetTopWorld());
						EditorFunctionCommand* EntityAttachCommand = CreateEntityDetachFromParentCommand((*payload_n));
						pActiveEditorWorld->GetCommandManager().AddCommand(EntityAttachCommand);
					}
				}
				else
				{
					EditorWorld* pActiveEditorWorld = dynamic_cast<EditorWorld*>(GetEditor()->GetApplication()->GetWorldManager().GetTopWorld());
					EditorFunctionCommand* EntityAttachCommand = CreateEntityAttachToParentCommand((*payload_n), pTargetEntity);
					pActiveEditorWorld->GetCommandManager().AddCommand(EntityAttachCommand);
				}

			}
		}

		ImGui::EndDragDropTarget();
	}
}

void EntityHierarchy::RenderEntitySearch(EntitySearchNode & rEntitySearchNode, Entity * pSelectedEntity)
{
	if (!rEntitySearchNode.RenderCurrentEntity)
		return;

	ImGuiTreeNodeFlags TreeNodeFlags = ImGuiTreeNodeFlags_OpenOnDoubleClick | ImGuiTreeNodeFlags_OpenOnArrow | ImGuiTreeNodeFlags_DefaultOpen;

	if (rEntitySearchNode.TargetEntity->GetChildren().empty() || !rEntitySearchNode.RenderAnyChildEntities)
		TreeNodeFlags = TreeNodeFlags | ImGuiTreeNodeFlags_Leaf;

	if (pSelectedEntity == rEntitySearchNode.TargetEntity)
		TreeNodeFlags = TreeNodeFlags | ImGuiTreeNodeFlags_Selected;

	const auto& io = ImGui::GetIO();

	if (ImGui::TreeNodeEx(GenerateSearchTreeNodeLabel(rEntitySearchNode.TargetEntity).c_str(), TreeNodeFlags))
	{
		// if we are dragging the entity around
		if (io.MouseDown[0] && ImGui::IsMouseDragging(0))
		{
			m_pSelectedEntity = nullptr;
			bClickOnEntity = false;
		}

		if (bClickOnEntity && io.MouseReleased[0])
		{
			GetEditor()->SetSelectedEntity(m_pSelectedEntity);
			bClickOnEntity = false;
		}

		if (IsCurrentTreeNodeClicked())
		{
			m_pSelectedEntity = rEntitySearchNode.TargetEntity;
			bClickOnEntity = true;
		}

		RenderPopUpEntityWindow(rEntitySearchNode.TargetEntity);

		ProcessEntityDragAndDrop(rEntitySearchNode.TargetEntity);

		if (rEntitySearchNode.RenderAnyChildEntities)
			RenderEntityChildrenSearch(rEntitySearchNode, pSelectedEntity);

		ImGui::TreePop();
	}
	else
	{
		// if we are dragging the entity around
		if (io.MouseDown[0] && ImGui::IsMouseDragging(0))
		{
			m_pSelectedEntity = nullptr;
			bClickOnEntity = false;
		}

		if (bClickOnEntity && io.MouseReleased[0])
		{
			GetEditor()->SetSelectedEntity(m_pSelectedEntity);
			bClickOnEntity = false;
		}

		if (IsCurrentTreeNodeClicked())
		{
			m_pSelectedEntity = rEntitySearchNode.TargetEntity;
			bClickOnEntity = true;
		}

		ProcessEntityDragAndDrop(rEntitySearchNode.TargetEntity);
	}
}

void EntityHierarchy::RenderEntityChildrenSearch(EntitySearchNode & rEntitySearchNode, Entity * pSelectedEntity)
{
	for (EntitySearchNode& ChildEntityNodes : rEntitySearchNode.ChildNodes)
		RenderEntitySearch(ChildEntityNodes, pSelectedEntity);
}

void EntityHierarchy::EntityNeedsToBeRendererd(Entity* pTargetEntity, EntitySearchNode& rEntitySearchNode, const std::string& NameSearch)
{
	std::string EntityName = pTargetEntity->GetName();
	std::transform(EntityName.begin(), EntityName.end(), EntityName.begin(), tolower);

	if (EntityName.find(NameSearch) != std::string::npos)
	{
		rEntitySearchNode.RenderCurrentEntity = true;

		EntitySearchNode* CurrentParentNode = rEntitySearchNode.ParentNode;

		while (CurrentParentNode != nullptr)
		{
			CurrentParentNode->RenderCurrentEntity = true;
			CurrentParentNode->RenderAnyChildEntities = true;
			CurrentParentNode = CurrentParentNode->ParentNode;
		}
	}

	const std::vector<Entity*> TargetEntityChildren = pTargetEntity->GetChildren();

	for (Entity* ChildEntity : TargetEntityChildren)
	{
		rEntitySearchNode.ChildNodes.push_back(EntitySearchNode(ChildEntity, &rEntitySearchNode));
		EntityNeedsToBeRendererd(ChildEntity, rEntitySearchNode.ChildNodes[rEntitySearchNode.ChildNodes.size() - 1], NameSearch);
	}
}

bool EntityHierarchy::IsCurrentTreeNodeClicked()
{
	return ImGui::IsItemClicked(0) && (ImGui::GetMousePos().x - ImGui::GetItemRectMin().x) > ImGui::GetTreeNodeToLabelSpacing();
}

std::string EntityHierarchy::GenerateTreeNodeLabel(Entity * pTargetEntity)
{
	return std::string(pTargetEntity->GetName() + "###Entity" + std::to_string(pTargetEntity->GetId()));
}

std::string EntityHierarchy::GenerateSearchTreeNodeLabel(Entity * pTargetEntity)
{
	return std::string(GenerateTreeNodeLabel(pTargetEntity) + "Search" + std::to_string(m_UniqueSearchID));
}
