#include "pch.h"
#include "Editor/EntityInspector/EntityInspector.h"

#include "External/imgui/imgui.h"
#include "External/rttr/type"
#include "External/rttr/filter_item.h"

#include "Editor/Editor.h"
#include "Editor/PropertyRenderer/PropertyRenderer.h"

#include "Core/ECS/Entity.h"
#include "Core/ECS/Component.h"
#include "Editor/PropertyRenderer/TransformMetaData.h"

#include "Core/Application.h"
#include "Core/ECS/WorldManager.h"
#include "Editor/EditorWorld.h"
#include <Core/Platform/Rendering/RenderContext.h>
#include <Core/Platform/Platform.h>

#include "Editor/UndoRedo/CommandManager.h"
#include "Editor/UndoRedo/EditorComponentCommand.h"
// #include "Editor/UndoRedo/EditorEntityChildCommand.h"

#include "Editor/imgui/List/ImList.h"

EntityInspector::EntityInspector()
	: Window()
	, m_bRenderAddComponentWindow(false)
{
}

EntityInspector::~EntityInspector()
{
}

void EntityInspector::Initialize(Editor * pTargetEditor)
{
	Window::Initialize(pTargetEditor);

	SetName("Entity Inspector");

	RenderContext* pRenderContext = GetEditor()->GetApplication()->GetPlatform().GetRenderContext();
	const UVector2 renderRes = pRenderContext->GetRenderResolution();

	SetSize(ImVec2(290.0f, static_cast<float>(renderRes.y) - 20.0f));
	SetPosition(ImVec2(static_cast<float>(renderRes.x) - GetSize().x, 26.0f));

	SetWindowFlag(ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoBringToFrontOnFocus);
}

void EntityInspector::UnInitialize()
{
}

void EntityInspector::Tick(float fDeltaTime)
{
	Window::Tick(fDeltaTime);
}

void EntityInspector::OnContextResize(uint32_t a_uiWidth, uint32_t a_uiHeight, IVector2 /* deltaResolution */)
{
	SetSize(ImVec2(GetSize().x, static_cast<float>(a_uiHeight) - 20.0f));
	SetPosition(ImVec2(static_cast<float>(a_uiWidth) - GetSize().x, 26.0f));

	ImGui::SetWindowPos(GetName().data(), GetPosition());
	ImGui::SetWindowSize(GetName().data(), GetSize());
}

void EntityInspector::RenderSelectedEntityProperty(rttr::instance& rInstance, rttr::property& rProperty, std::string* pCategory)
{
	const std::string PropertyLabel = "###" + rInstance.get_derived_type().get_name().to_string() + rProperty.get_name().to_string() + std::to_string(GetEditor()->GetSelectedEntity()->GetId());

	if (rProperty.get_metadata("Category").is_valid() && !rProperty.get_metadata("Category").get_value<std::string>().empty())
	{
		m_CategoryOrganizerMap.insert(std::make_pair(
			rProperty.get_metadata("Category").get_value<std::string>(),
			PropertyCategory{ rInstance, rProperty, PropertyLabel }
		));
	} 
	else
	{
		m_CategoryOrganizerMap.insert(std::make_pair(
			(pCategory) ? *pCategory : "Default",
			PropertyCategory{ rInstance, rProperty, PropertyLabel, true, rttr::type::get<Component*>().is_base_of(rInstance.get_type()) }
		));
	}
}

void EntityInspector::OnPreRender(float /* fDeltaTime */ )
{
	ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(17.0f, 5.0f));

	if (GetEditor()->HasSelectedEntity())
	{
		Entity* SelectedEntity = GetEditor()->GetSelectedEntity();
		std::vector<Component*> InspectedComponents = SelectedEntity->GetComponents();

		// Find the Transform component en remove it from the to render list
		InspectedComponents.erase(std::remove(InspectedComponents.begin(), InspectedComponents.end(), SelectedEntity->GetTransform()), InspectedComponents.end());
		
		rttr::instance InspectedEntity = *SelectedEntity;
		rttr::type EntityType = InspectedEntity.get_derived_type();

		// we need to reorganize the properties that we are going to render.
		for (rttr::property EntityProp : EntityType.get_properties())
		{
			RenderSelectedEntityProperty(InspectedEntity, EntityProp);
		}

		// Transform property grabbing
		rttr::instance InspectedTransform = rttr::variant(SelectedEntity->GetTransform());
		rttr::type TransformComponentType = InspectedTransform.get_derived_type();
		std::string pComponentName = TransformComponentType.get_name().to_string() + "##" + std::to_string(SelectedEntity->GetId());

		const bool IsTransformWorldModus = GetEditor()->IsTransformWorldModus();

		for (rttr::property ComponentProp : TransformComponentType.get_properties())
		{
			if (ComponentProp.get_metadata(MetaData_Transform_Mode::GLOBAL) || ComponentProp.get_metadata(MetaData_Transform_Mode::LOCAL))
			{
				if (IsTransformWorldModus && ComponentProp.get_metadata(MetaData_Transform_Mode::GLOBAL) ||
					!IsTransformWorldModus && ComponentProp.get_metadata(MetaData_Transform_Mode::LOCAL))
				{
					RenderSelectedEntityProperty(InspectedTransform, ComponentProp, &pComponentName);
				}

				continue;
			}

			if (ComponentProp.get_name() != "Enabled")
				RenderSelectedEntityProperty(InspectedTransform, ComponentProp, &pComponentName);
		}

		for (Component* it : InspectedComponents)
		{
			rttr::instance InspectedComponent = it;
			rttr::type ComponentType = rttr::type::get(*it);

			pComponentName = ComponentType.get_name().to_string() + "##" + std::to_string(SelectedEntity->GetId());

			for (rttr::property ComponentProp : ComponentType.get_properties())
				RenderSelectedEntityProperty(InspectedComponent, ComponentProp, &pComponentName);
		}
	}
}

void EntityInspector::OnRender(float /* fDeltaTime */)
{
	ImGui::PopStyleVar();

	/*if (ImGui::CollapsingHeader("Project Settings ##", nullptr, ImGuiTreeNodeFlags_DefaultOpen))
	{
		ImGui::Text("%s", GetEditor()->GetProjectSettings().GetDefaultMap().c_str());
		if (ImGui::Button("Set Level ##"))
		{
			if (!GetEditor()->GeEditorWorld().empty())
				GetEditor()->GetProjectSettings().SetDefaultMap(GetEditor()->GeEditorWorld());
			else
				GetEditor()->GetApplication()->GetLoggingSystem().Log(LOGLEVEL_WARNING, "Can't set empty string to level");
		}
		ImGui::SameLine();
		if (ImGui::Button("Remove default ##"))
		{
			GetEditor()->GetProjectSettings().SetDefaultMap("");
		}
	}*/

	if (GetEditor()->HasSelectedEntity())
	{
		Entity* SelectedEntity = GetEditor()->GetSelectedEntity();
		std::vector<Component*> InspectedComponents = SelectedEntity->GetComponents();
		InspectedComponents.erase(std::remove(InspectedComponents.begin(), InspectedComponents.end(), SelectedEntity->GetTransform()), InspectedComponents.end());

		if (ImGui::CollapsingHeader(("Transform##" + std::to_string(SelectedEntity->GetId())).data(), &m_bRenderDefaultCategory, ImGuiTreeNodeFlags_DefaultOpen))
		{
			if (ImGui::Selectable("World", GetEditor()->IsTransformWorldModus(), 0, ImVec2(130, 16)))
			{
				GetEditor()->SetTransformWorldModus(true);
			}

			ImGui::SameLine();

			if (ImGui::Selectable("Local", !GetEditor()->IsTransformWorldModus(), 0, ImVec2(130, 16)))
			{
				GetEditor()->SetTransformWorldModus(false);
			}

			const auto& itr = m_CategoryOrganizerMap.equal_range("Transform##" + std::to_string(SelectedEntity->GetId()));
			for (std::unordered_multimap<std::string, PropertyCategory>::iterator it = itr.first; it != itr.second; ++it)
			{
				PropertyCategory& cat = it->second;

				GetEditor()->GetPropertyRenderer().Render(cat.Instance, cat.Property, cat.StringLabel);
			}
		}

		if (ImGui::CollapsingHeader(("Default ##" + std::to_string(SelectedEntity->GetId())).data(), &m_bRenderDefaultCategory, ImGuiTreeNodeFlags_DefaultOpen))
		{
			const auto& itr = m_CategoryOrganizerMap.equal_range("Default");
			for (std::unordered_multimap<std::string, PropertyCategory>::iterator it = itr.first; it != itr.second; ++it)
			{
				PropertyCategory& cat = it->second;

				GetEditor()->GetPropertyRenderer().Render(cat.Instance, cat.Property, cat.StringLabel);
			}
		}

		if (!m_bRenderDefaultCategory)
			m_bRenderDefaultCategory = true;

		ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.4f, 0.505f, 0.243f, 0.9f));
		ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.5f, 0.605f, 0.343f, 0.9f));
		ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.3f, 0.405f, 0.143f, 0.9f));

		if (ImGui::Button("Add Component", ImVec2(275, 32)))
		{
			m_bRenderAddComponentWindow = !m_bRenderAddComponentWindow;
			m_SearchStringAddComponent.clear();
			ImGui::OpenPopup("EntityInspector_Add_Component");
		}
		ImGui::PopStyleColor(3);

		const ImVec2 ButtonMin = ImGui::GetItemRectMin();
		const ImVec2 ButtonMax = ImGui::GetItemRectMax();

		if (m_bRenderAddComponentWindow)
		{
			ImGui::SetNextWindowPos(ImVec2(ButtonMin.x, ButtonMax.y));
			ImGui::SetNextWindowSize(ImVec2(ButtonMax.x - ButtonMin.x, 128));

			if (ImGui::BeginPopup("EntityInspector_Add_Component"))
			{
				ImGui::PushItemWidth(ButtonMax.x - ButtonMin.x);

				if (ImGui::IsRootWindowOrAnyChildFocused() && !ImGui::IsAnyItemActive() && !ImGui::IsMouseClicked(0))
					ImGui::SetKeyboardFocusHere(0);

				auto ComponentTypes = rttr::type::get<Component>().get_derived_classes();
				uint32_t uiIndex = 0; // huidige keuze
				ImList<std::string> ValidComponentsToAddDropdown("AddComponent");

				for (auto i = ComponentTypes.begin(); i != ComponentTypes.end(); ++i)
				{
					if (*i != rttr::type::get<Transform>() && i->get_constructors().size() != 0)
					{
						bool EntityContainsComponent = false;

						for (auto && c : InspectedComponents)
						{
							if (*i == rttr::type::get(*c))
							{
								EntityContainsComponent = true;
								break;
							}
						}

						if (!EntityContainsComponent)
							ValidComponentsToAddDropdown.AddValue(i->get_name().to_string(), i->get_name().to_string());
					}
				}

				ValidComponentsToAddDropdown.SetSearchValue(m_SearchStringAddComponent.c_str());
				ValidComponentsToAddDropdown.SetCurrentValue(uiIndex);

				ValidComponentsToAddDropdown.AddListener([&](const std::string&, const std::string& selectedType)
				{
					rttr::type NewComponentType = rttr::type::get_by_name(selectedType);

					if (NewComponentType)
					{
						EditorWorld* pActiveEditorWorld = dynamic_cast<EditorWorld*>(GetEditor()->GetApplication()->GetWorldManager().GetTopWorld());
						EditorComponentCommand* pEditorEntityCommand = CreateComponentCreationCommand(NewComponentType, SelectedEntity);
						pActiveEditorWorld->GetCommandManager().AddCommand(pEditorEntityCommand);
					}

					m_bRenderAddComponentWindow = false;
				});

				ValidComponentsToAddDropdown.Draw();
				m_SearchStringAddComponent = ValidComponentsToAddDropdown.GetSearchValue();

				ImVec2 WindowSize = ImGui::GetWindowSize();
				ImVec2 MousPos = ImGui::GetMousePos();
				auto test = ImVec2(ButtonMin.x + WindowSize.x, (ButtonMax.y - ButtonMin.y) + ButtonMin.y + WindowSize.y);

				if (MousPos.x < ButtonMin.x || MousPos.x > ButtonMin.x + WindowSize.x || MousPos.y < ButtonMin.y || MousPos.y >(ButtonMax.y - ButtonMin.y) + ButtonMin.y + WindowSize.y)
				{
					m_bRenderAddComponentWindow = false;
				}

				if (MousPos.x < ButtonMax.x && MousPos.x > ButtonMin.x && MousPos.y < ButtonMax.y && MousPos.y > ButtonMin.y)
				{
					if (ImGui::IsMouseClicked(0))
						m_bRenderAddComponentWindow = false;
				}

				ImGui::EndPopup();
			}
		}

		std::pair<int, std::string> pair = { 0, "" };
		bool bOpened = false;
		for (auto& itr : m_CategoryOrganizerMap)
		{
			if (itr.first == "Default" || itr.first == "Transform##" + std::to_string(SelectedEntity->GetId()))
				continue;

				PropertyCategory & cat = itr.second;

				// if we need to render something different
				if (pair.second != itr.first)
				{
					pair.first = 0;
				}

			if (!pair.first)
			{
				bOpened = ImGui::CollapsingHeader(itr.first.data(), &cat.bKeepComponentAlive);

				pair.first++;
				pair.second = itr.first;
			}

			if (bOpened)
			{
				GetEditor()->GetPropertyRenderer().Render(cat.Instance, cat.Property, cat.StringLabel);
			}


			if (!cat.bKeepComponentAlive && cat.bIsComponent && cat.Instance.get_derived_type() != rttr::type::get<Transform>())
			{
				const auto& pComponent = cat.Instance.try_convert<Component>();
				if (pComponent)
				{
					auto pActiveEditorWorld = dynamic_cast<EditorWorld*>(GetEditor()->GetApplication()->GetWorldManager().GetTopWorld());
					EditorComponentCommand* pEditorEntityCommand = CreateComponentDestroyCommand(pComponent);
					pActiveEditorWorld->GetCommandManager().AddCommand(pEditorEntityCommand);
				}
			}
			else // we are dealing with a custom category
			{
				cat.bKeepComponentAlive = true;
			}
		}

		// Clear out the rendering process
		m_CategoryOrganizerMap.clear();
	}
}

void EntityInspector::RenderCategories(std::unordered_multimap<std::string, PropertyCategory>& CategoryOrganizerMap)
{

}

