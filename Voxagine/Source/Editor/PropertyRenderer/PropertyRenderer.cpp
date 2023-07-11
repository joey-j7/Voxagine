#include "pch.h"
#include "Editor/PropertyRenderer/PropertyRenderer.h"
#include "Editor/PropertyRenderer/TMap.h"

#include "External/imgui/imgui.h"
#include "External/imgui/imgui_stl.h"
#include "External/imgui/imgui_internal.h"

#include "Editor/Editor.h"
#include "Editor/EditorWorld.h"
#include "Editor/UndoRedo/EditorPropertyCommand.h"
#include "Core/Application.h"
#include "Core/Resources/ResourceManager.h"
#include "Core/LoggingSystem/LoggingSystem.h"

#include "Core/MetaData/PropertyTypeMetaData.h"

#include "Core/Math.h"
#include "Editor/imgui/List/ImList.h"

#include "Core/Objects/TSubclass.h"

PropertyRenderer::PropertyRenderer() { }

PropertyRenderer::~PropertyRenderer() { }

void PropertyRenderer::Initialize(Editor * pEditor)
{
	m_pEditor = pEditor;

	InitializePropertyLookUp();
	InitializeResourcePropertyLookUp();

	m_NotSupportedProperty = [](rttr::instance&, rttr::property&, std::string&, bool*) 
	{
		ImGui::PushTextWrapPos(-1.f);
		ImGui::TextColored(ImVec4(1.f, 0.f, 0.f, 1.f), "Unsupported Property");
		ImGui::PopTextWrapPos();
	};
}

void PropertyRenderer::Render(rttr::instance & rInstance, rttr::property & rProperty, bool* bIsElementAndChanged)
{
	Render(rInstance, rProperty, rProperty.get_name().to_string(), bIsElementAndChanged);
}

void PropertyRenderer::Render(rttr::instance & rInstance, rttr::property & rProperty, std::string Label, bool* bIsElementAndChanged)
{
	if (!rProperty.get_metadata(RTTR_ACCESS_MODIFIER).is_valid() || rProperty.get_metadata(RTTR_ACCESS_MODIFIER).get_value<RttrAccessModifier>() == RttrAccessModifier::RAM_PRIVATE)
		return;

	ImGui::Columns(2, "##EntityInspector_", true);  // 3-ways, no border
	ImGui::PushItemWidth(200.0f);

	rttr::variant PropertyValueVar = rProperty.get_value(rInstance);

	if(rttr::type::get<VClass>().is_base_of(rProperty.get_type()) && !PropertyValueVar.get_type().invoke("IsSubClass", PropertyValueVar, {}).to_bool())
	{
		ImGui::Columns(1);

		// see if it is an v_object
		ImGuiTreeNodeFlags TreeNodeFlags = ImGuiTreeNodeFlags_OpenOnDoubleClick | ImGuiTreeNodeFlags_OpenOnArrow;

		if (rProperty.get_type().get_properties().empty())
			TreeNodeFlags = TreeNodeFlags | ImGuiTreeNodeFlags_Leaf;

		if (ImGui::TreeNodeEx(rProperty.get_name().to_string().c_str(), TreeNodeFlags))
		{
			ImGui::NextColumn();
			RenderProperty(rInstance, rProperty, Label, bIsElementAndChanged);

			ImGui::TreePop();
		}

		ImGui::Columns(2);
	}
	else
	{
		ImGui::Text("%s", rProperty.get_name().data());
		ImGui::SameLine();

		// Tooltip information for the player
#ifdef EDITOR
		Utils::Tooltip(rProperty);
#endif

		ImGui::NextColumn();

		if (!rProperty.get_metadata(RttrPropertyType::RPT_RESOURCE).is_valid())
		{
			// check if it is a enum
			if (rProperty.is_enumeration())
			{
				RenderEnumProperty(rInstance, rProperty, Label, bIsElementAndChanged);
			}
			else
			{
				RenderProperty(rInstance, rProperty, Label, bIsElementAndChanged);
			}

			ImGui::NextColumn();
		}
		else
		{
			rttr::variant MetaDataVariant = rProperty.get_metadata(RttrPropertyType::RPT_RESOURCE);
			const std::string& FileExtension = MetaDataVariant.get_value<std::string>();

			if (rProperty.get_type().is_sequential_container())
				RenderArrayResource(FileExtension, rInstance, rProperty, Label);
			else
				RenderResource(FileExtension, rInstance, rProperty, Label, bIsElementAndChanged);

			ImGui::NextColumn();
		}
	}

	ImGui::Columns(1);
	ImGui::Separator();
}

void PropertyRenderer::InitializePropertyLookUp()
{
	// IMGUI ENTITY Rendering
	CreatePropertyLookUp<Entity>([=](rttr::instance& rInstance, rttr::property& rProperty, const std::string& rLabel, bool* bIsElementAndChanged)
	{
		rttr::variant rTempValueVar = rProperty.get_value(rInstance);
		rttr::instance rEntityInstance = rTempValueVar;

		ImGui::Text("%s", rProperty.get_name().to_string().c_str());

		for (auto prop : rEntityInstance.get_derived_type().get_properties())
		{
			Render(rEntityInstance, prop);
		}

		if (!rProperty.set_value(rInstance, rTempValueVar))
		{
			m_pEditor->GetApplication()->GetLoggingSystem().Log(LogLevel::LOGLEVEL_WARNING, "PropertyRenderer", "Unable to set value in Entity! : " + rLabel);
		}
		// This is purely if the entity lookup is rendered inside an array
		else if (bIsElementAndChanged) *bIsElementAndChanged = true;
	});

	// IMGUI ENTITY POINTER Rendering
	CreatePropertyLookUp<Entity*>([=](rttr::instance& rInstance, rttr::property& rProperty, const std::string& rLabel, bool* bIsElementAndChanged)
	{
		const ImGuiInputTextFlags PropertyFlags = ImGuiInputTextFlags_ReadOnly;

		Entity* TempValue = rProperty.get_value(rInstance).get_value<Entity*>();

		std::string tempValueText = TempValue ? TempValue->GetName() : std::string("None");

		ImGui::InputText(("##EntitySource_" + rLabel + "_" + rProperty.get_name().to_string()).c_str(), &tempValueText, PropertyFlags);

		if (ImGui::BeginDragDropTarget())
		{
			ImGuiStyle& style = ImGui::GetStyle();
			if (const ImGuiPayload * payload = ImGui::AcceptDragDropPayload("Entity_Drag-And-Drop", ImGuiDragDropFlags_AcceptBeforeDelivery))
			{
				// we can now control if we could actually override it.
				const auto payload_n = static_cast<Entity * *>(payload->Data);
				if (payload_n != nullptr)
				{
					// first check if we just need the base class
					// second so the thing that we want is not the base type that we need
					rttr::type propertyType = rProperty.get_type();
					const auto& infoType = (*payload_n)->get_derived_info().m_type;
					if (propertyType != rttr::type::get<Entity*>() && // check if the type is entity 
						(propertyType.is_pointer() ? propertyType.get_raw_type() : propertyType) != infoType && // check if the type is the same type
						!propertyType.is_base_of((*payload_n)->get_type())) // check if the type is derived of the property type
					{
						style.Colors[ImGuiCol_DragDropTarget] = ImVec4(1.0f, 0.0f, 0.0f, 1.0f);
						return;
					}

					style.Colors[ImGuiCol_DragDropTarget] = ImVec4(0.0f, 1.0f, 0.0f, 1.0f);
					if (payload->IsDelivery())
					{
						rttr::variant varComponent = *payload_n;
						varComponent.convert(rProperty.get_type());
						if (!rProperty.set_value(rInstance, varComponent))
						{
							m_pEditor->GetApplication()->GetLoggingSystem().Log(LogLevel::LOGLEVEL_WARNING, "PropertyRenderer", "Unable to set value in type! : " + rLabel);
						}
						else
						{
							if (bIsElementAndChanged) *bIsElementAndChanged = true;
							FirePropertyCommand(rInstance, rProperty, varComponent);
						}
					}
				}
			}

			ImGui::EndDragDropTarget();
		}
	});

	// IMGUI Component Rendering
	CreatePropertyLookUp<Component>([=](rttr::instance& rInstance, rttr::property& rProperty, const std::string& rLabel, bool* bIsElementAndChanged)
	{
		const ImGuiInputTextFlags PropertyFlags = ImGuiInputTextFlags_ReadOnly;

		rttr::variant rTempValueVar = rProperty.get_value(rInstance);
		rttr::instance rCompInstance = rTempValueVar;

		ImGui::Text("%s", rProperty.get_name().to_string().c_str());

		for (auto prop : rCompInstance.get_derived_type().get_properties())
		{
			Render(rCompInstance, prop);
		}

		if (!rProperty.set_value(rInstance, rTempValueVar))
		{
			m_pEditor->GetApplication()->GetLoggingSystem().Log(LogLevel::LOGLEVEL_WARNING, "PropertyRenderer", "Unable to set value in type! : " + rLabel);
		} else if (bIsElementAndChanged) *bIsElementAndChanged = true;
	});

	// IMGUI Component Pointer Rendering
	CreatePropertyLookUp<Component*>([=](rttr::instance& rInstance, rttr::property& rProperty, const std::string& rLabel, bool* bIsElementAndChanged)
	{
		const ImGuiInputTextFlags PropertyFlags = ImGuiInputTextFlags_ReadOnly;

		Component* TempValue = rProperty.get_value(rInstance).get_value<Component*>();

		std::string tempValueText = TempValue ? TempValue->GetOwner()->GetName() + " (" + TempValue->get_type().get_raw_type().get_name().to_string() + ")" : std::string("None");

		ImGui::InputText(("##EntitySource" + rProperty.get_name().to_string()).c_str(), &tempValueText, PropertyFlags);

		if (ImGui::BeginDragDropTarget())
		{
			ImGuiStyle& style = ImGui::GetStyle();
			if (const ImGuiPayload * payload = ImGui::AcceptDragDropPayload("Entity_Drag-And-Drop", ImGuiDragDropFlags_AcceptBeforeDelivery))
			{
				// we can now control if we could actually override it.
				const auto payload_n = static_cast<Entity * *>(payload->Data);
				if (payload_n != nullptr)
				{
					// let's see if the entity has this component
					if (Component * pComponent = (*payload_n)->GetComponent(rProperty.get_type()))
					{
						// first check if we just need the base class
						// second so the thing that we want is not the base type that we need
						const rttr::type propertyType = rProperty.get_type();
						const auto& infoType = pComponent->get_derived_info().m_type;
						if (propertyType != rttr::type::get<Component*>() && // check if the type is entity 
							(propertyType.is_pointer() ? propertyType.get_raw_type() : propertyType) != infoType && // check if the type is the same type
							!propertyType.is_base_of((*payload_n)->get_type())) // check if the type is derived of the property type
						{
							style.Colors[ImGuiCol_DragDropTarget] = ImVec4(1.0f, 0.0f, 0.0f, 1.0f);
						}


						style.Colors[ImGuiCol_DragDropTarget] = ImVec4(0.0f, 1.0f, 0.0f, 1.0f);
						if (payload->IsDelivery())
						{
							rttr::variant varComponent = pComponent;
							varComponent.convert(rProperty.get_type());
							if (!rProperty.set_value(rInstance, varComponent))
							{
								m_pEditor->GetApplication()->GetLoggingSystem().Log(LogLevel::LOGLEVEL_WARNING, "PropertyRenderer", "Unable to set value in type! : " + rLabel);
							}
							else
							{
								if (bIsElementAndChanged) *bIsElementAndChanged = true;
								FirePropertyCommand(rInstance, rProperty, varComponent);
							}
						}
					}
				}
			}

			ImGui::EndDragDropTarget();
		}
	});

	// IMGUI STRING Rendering
	CreatePropertyLookUp<std::string>([this](rttr::instance& rInstance, rttr::property& rProperty, std::string& rLabel, bool* bIsElementAndChanged)
	{
		ImGuiInputTextFlags PropertyFlags = 0;

		if (rProperty.is_readonly())
			PropertyFlags = PropertyFlags | ImGuiInputTextFlags_ReadOnly;

		std::string TempValue = rProperty.get_value(rInstance).to_string();

		if (ImGui::InputText(rLabel.data(), &TempValue, PropertyFlags))
		{
			if (bIsElementAndChanged) *bIsElementAndChanged = true;

			rttr::variant NewValue = TempValue;
			FirePropertyCommand(rInstance, rProperty, NewValue);
		}
	});

	// IMGUI VECTOR2 Rendering
	CreatePropertyLookUp<Vector2>([this](rttr::instance& rInstance, rttr::property& rProperty, std::string& rLabel, bool* bIsElementAndChanged)
	{
		ImGuiInputTextFlags PropertyFlags = ImGuiInputTextFlags_EnterReturnsTrue;

		if (rProperty.is_readonly())
			PropertyFlags = PropertyFlags | ImGuiInputTextFlags_ReadOnly;

		Vector2 TempValue = rProperty.get_value(rInstance).convert<Vector2>();
		float TempArrayValues[] = { TempValue.x, TempValue.y };

		if (ImGui::InputFloat2(rLabel.data(), TempArrayValues, 4, PropertyFlags))
		{
			if (bIsElementAndChanged)* bIsElementAndChanged = true;

			rttr::variant NewValue = Vector2(TempArrayValues[0], TempArrayValues[1]);
			FirePropertyCommand(rInstance, rProperty, NewValue);
		}
	});

	// IMGUI VECTOR3 Rendering
	CreatePropertyLookUp<Vector3>([this](rttr::instance& rInstance, rttr::property& rProperty, std::string& rLabel, bool* bIsElementAndChanged)
	{
		ImGuiInputTextFlags PropertyFlags = ImGuiInputTextFlags_EnterReturnsTrue;

		if (rProperty.is_readonly())
			PropertyFlags = PropertyFlags | ImGuiInputTextFlags_ReadOnly;

		Vector3 TempValue = rProperty.get_value(rInstance).convert<Vector3>();
		float TempArrayValues[] = { TempValue.x, TempValue.y, TempValue.z };

		if (ImGui::InputFloat3(rLabel.data(), TempArrayValues, 4, PropertyFlags))
		{
			if (bIsElementAndChanged)* bIsElementAndChanged = true;

			rttr::variant NewValue = Vector3(TempArrayValues[0], TempArrayValues[1], TempArrayValues[2]);
			FirePropertyCommand(rInstance, rProperty, NewValue);
		}
	});

	// IMGUI VECTOR4 Rendering
	CreatePropertyLookUp<Vector4>([this](rttr::instance& rInstance, rttr::property& rProperty, std::string& rLabel, bool* bIsElementAndChanged)
	{
		ImGuiInputTextFlags PropertyFlags = ImGuiInputTextFlags_EnterReturnsTrue;

		if (rProperty.is_readonly())
			PropertyFlags = PropertyFlags | ImGuiInputTextFlags_ReadOnly;

		Vector4 TempValue = rProperty.get_value(rInstance).convert<Vector4>();
		float TempArrayValues[] = { TempValue.x, TempValue.y, TempValue.z, TempValue.w };

		if (ImGui::InputFloat4(rLabel.data(), TempArrayValues, 4, PropertyFlags))
		{
			if (bIsElementAndChanged)* bIsElementAndChanged = true;

			rttr::variant NewValue = Vector4(TempArrayValues[0], TempArrayValues[1], TempArrayValues[2], TempArrayValues[3]);
			FirePropertyCommand(rInstance, rProperty, NewValue);
		}
	});

	// IMGUI VECTOR4 Rendering
	CreatePropertyLookUp<VColor>([this](rttr::instance& rInstance, rttr::property& rProperty, std::string& rLabel, bool* bIsElementAndChanged)
	{
		ImGuiInputTextFlags PropertyFlags = ImGuiInputTextFlags_EnterReturnsTrue;

		if (rProperty.is_readonly())
			PropertyFlags = PropertyFlags | ImGuiColorEditFlags_NoInputs;

		VColor TempValue = rProperty.get_value(rInstance).convert<VColor>();
		const auto TempRGBA = TempValue.inst.Color;

		const auto c4 = ImGui::ColorConvertU32ToFloat4(TempRGBA);
		float eColor[4] = 
		{
			c4.x,
			c4.y,
			c4.z,
			c4.w
		};

		if (ImGui::ColorEdit4(rLabel.data(), eColor, PropertyFlags))
		{
			if (bIsElementAndChanged)* bIsElementAndChanged = true;

			TempValue.inst.Color = ImGui::ColorConvertFloat4ToU32(ImVec4(eColor[0], eColor[1], eColor[2], eColor[3]));

			rttr::variant NewValue = TempValue;
			FirePropertyCommand(rInstance, rProperty, NewValue);
		}
	});

	// IMGUI VClass/TSUBCLASS Rendering
	CreatePropertyLookUp<VClass>([=](rttr::instance& rInstance, rttr::property& rProperty, const std::string& rLabel, bool* bIsElementAndChanged)
	{
		// Grab the actual TSubClass or struct, class
		rttr::variant TempValueVar = rProperty.get_value(rInstance);
		// Grab the base class to grab the inner class
		VClass TempValue = rProperty.get_value(rInstance).get_value<VClass>();

		// If it is a TSubclass do something different
		if (TempValue.is_sub_class())
		{
			// Subclass ID title
			const std::string SubclassSetter = "SubclassSetter";
			const ImGuiInputTextFlags PropertyFlags = ImGuiInputTextFlags_ReadOnly;

			const ImGuiID PopUpID = ImGui::GetID(SubclassSetter.c_str());
			const bool PopUpOpen = ImGui::IsPopupOpen(SubclassSetter.c_str());

			const auto derived_classes = TempValueVar.get_type().invoke("GetDerivedTypes", TempValueVar, {}).get_value<rttr::array_range<rttr::type>>();

			// If we have the base selected then render none.
			if (ImGui::Selectable(((TempValueVar.get_type().invoke("IsSame", TempValueVar, {}).to_bool() ? TempValue.get_inner_type().get_name().to_string() : TempValue.get_inner_type().get_name().to_string()) + "###" + rLabel).c_str()))
			{
				if (!PopUpOpen)
				{
					ImGui::OpenPopup(SubclassSetter.c_str());
					m_PopUpSearchString.clear();
				}
			}

			// None make a big dropdown list
			uint32_t uiIndex = 0;
			ImList<std::string> SubClassDropdown("Entity_" + TempValue.get_type().get_name().to_string() + "_Source_Type_", PropertyFlags);

			int counter = 0;
			for (const auto& derived_class : derived_classes)
			{
				SubClassDropdown.AddValue(derived_class.get_raw_type().get_name().to_string(), derived_class.get_name().to_string());
				counter++;

#ifdef EDITOR
				if (TempValue.m_InstantiateType == derived_class.get_name().to_string())
					uiIndex = counter;
#endif
			}

			// if it is empty we don't have derived classes.
			if (derived_classes.empty())
			{
				uiIndex = ++counter;
				SubClassDropdown.AddValue(TempValue.get_inner_type().get_raw_type().get_name().to_string(), TempValue.get_inner_type().get_name().to_string());
			}

			const ImVec2 SelectableSize = ImGui::GetItemRectSize();
			ImGui::SetNextWindowSize(ImVec2(SelectableSize.x, SelectableSize.y * 10));

			if (ImGui::BeginPopup(SubclassSetter.c_str()))
			{
				ImGui::PushItemWidth(SelectableSize.x);

				if (ImGui::IsRootWindowOrAnyChildFocused() && !ImGui::IsAnyItemActive() && !ImGui::IsMouseClicked(0))
					ImGui::SetKeyboardFocusHere(0);

				// Set the search value
				SubClassDropdown.SetSearchValue(m_PopUpSearchString.c_str());
				SubClassDropdown.SetCurrentValue(uiIndex);
				SubClassDropdown.AddListener([&](const std::string&, const std::string& selectedType)
				{
					rttr::type NewSelectedType = rttr::type::get_by_name(selectedType);
#ifdef EDITOR
					if (TempValue.m_InstantiateType == NewSelectedType.get_name().to_string())
					{
						ImGui::CloseCurrentPopup();
						return;
					}
#else
					if (rttr::type::get_by_name(selectedType).get_name().to_string().empty())
						return;
#endif
					const rttr::instance instValue = TempValueVar;
					TempValueVar.get_type().invoke("SetInstantiateType", instValue, { NewSelectedType });

					FirePropertyCommand(rInstance, rProperty, TempValueVar);

					ImGui::CloseCurrentPopup();
				});

				SubClassDropdown.Draw();

				// after the draw we need to grab the search value
				m_PopUpSearchString = SubClassDropdown.GetSearchValue();

				ImGui::EndPopup();
			}
		}
		else
		{
			rttr::instance vClassInstance = TempValueVar;
			for (auto prop : vClassInstance.get_derived_type().get_properties())
			{
				const std::string elPropName = "##_" + rLabel + "_" + prop.get_name().to_string() + "_";
				Render(vClassInstance, prop, elPropName, bIsElementAndChanged);
			}

			if (!rProperty.set_value(rInstance, TempValueVar))
			{
				m_pEditor->GetApplication()->GetLoggingSystem().Log(LogLevel::LOGLEVEL_WARNING, "PropertyRenderer", "Unable to set value in type! : " + rLabel);
			}
		}
	});

	// IMGUI BOOL Rendering
	CreatePropertyLookUp<bool>([this](rttr::instance& rInstance, rttr::property& rProperty, std::string& rLabel, bool* bIsElementAndChanged)
	{
		bool TempValue = rProperty.get_value(rInstance).to_bool();

		if (rProperty.is_readonly())
		{
			ImGui::Checkbox(rLabel.data(), &TempValue);
			if (rProperty.is_readonly())
			{
				ImGui::PushItemFlag(ImGuiItemFlags_Disabled, true);
				ImGui::PushStyleVar(ImGuiStyleVar_Alpha, ImGui::GetStyle().Alpha * 0.5f);
			}
			if (rProperty.is_readonly())
			{
				ImGui::PopItemFlag();
				ImGui::PopStyleVar();
			}
		}
		else
		{
			if (ImGui::Checkbox(rLabel.data(), &TempValue))
			{
				if (bIsElementAndChanged)* bIsElementAndChanged = true;

				rttr::variant NewValue = TempValue;
				FirePropertyCommand(rInstance, rProperty, NewValue);
			}
		}
	});

	// IMGUI Unsigned int Rendering
	CreatePropertyLookUp<unsigned int>([this](rttr::instance& rInstance, rttr::property& rProperty, std::string& rLabel, bool* bIsElementAndChanged)
	{
		ImGuiInputTextFlags PropertyFlags = ImGuiInputTextFlags_EnterReturnsTrue;

		if (rProperty.is_readonly())
			PropertyFlags = PropertyFlags | ImGuiInputTextFlags_ReadOnly;

		unsigned int TempValue = rProperty.get_value(rInstance).to_uint32();
		auto TempIntValue = static_cast<int>(TempValue);

		if (ImGui::InputInt(rLabel.data(), &TempIntValue, 1, 100, PropertyFlags))
		{
			if (TempIntValue < 0)
				TempIntValue = 0;
			else if (TempIntValue > USHRT_MAX)
				TempIntValue = USHRT_MAX;

			if (bIsElementAndChanged) *bIsElementAndChanged = true;

			rttr::variant NewValue = static_cast<unsigned int>(TempIntValue);
			FirePropertyCommand(rInstance, rProperty, NewValue);
		}
	});

	// IMGUI Unsigned int64_t Rendering
	CreatePropertyLookUp<size_t>([this](rttr::instance& rInstance, rttr::property& rProperty, std::string& rLabel, bool* bIsElementAndChanged)
	{
		size_t TempValue = rProperty.get_value(rInstance).to_uint64();
		auto TempIntValue = static_cast<int64_t>(TempValue);

		auto max = std::numeric_limits<std::size_t>::max();
		if (ImGui::DragScalar(rLabel.data(), ImGuiDataType_U64, &TempIntValue, 1.0f, nullptr, &max))
		{
			if (!rProperty.is_readonly())
			{
				if (TempIntValue < 0)
					TempIntValue = 0;
				else if (TempIntValue > static_cast<int64_t>(std::numeric_limits<std::size_t>::max()))
					TempIntValue = std::numeric_limits<std::size_t>::max();

				if (bIsElementAndChanged)* bIsElementAndChanged = true;

				rttr::variant NewValue = static_cast<size_t>(TempIntValue);
				FirePropertyCommand(rInstance, rProperty, NewValue);
			}
		}
	});

	// IMGUI float Rendering
	CreatePropertyLookUp<float>([this](rttr::instance& rInstance, rttr::property& rProperty, std::string& rLabel, bool* bIsElementAndChanged)
	{
		ImGuiInputTextFlags PropertyFlags = ImGuiInputTextFlags_EnterReturnsTrue;

		if (rProperty.is_readonly())
			PropertyFlags = PropertyFlags | ImGuiInputTextFlags_ReadOnly;

		float TempValue = rProperty.get_value(rInstance).to_float();

		if (ImGui::InputFloat(rLabel.data(), &TempValue, 1.f, 100.f, 4, PropertyFlags))
		{
			if (bIsElementAndChanged)* bIsElementAndChanged = true;

			rttr::variant NewValue = TempValue;
			FirePropertyCommand(rInstance, rProperty, NewValue);
		}
	});

	// IMGUI INT Rendering
	CreatePropertyLookUp<int>([this](rttr::instance& rInstance, rttr::property& rProperty, std::string& rLabel, bool* bIsElementAndChanged)
	{
		ImGuiInputTextFlags PropertyFlags = ImGuiInputTextFlags_EnterReturnsTrue;

		if (rProperty.is_readonly())
			PropertyFlags = PropertyFlags | ImGuiInputTextFlags_ReadOnly;

		int TempValue = rProperty.get_value(rInstance).to_int();

		if (ImGui::InputInt(rLabel.data(), &TempValue, 1, 100, PropertyFlags))
		{
			if (bIsElementAndChanged)* bIsElementAndChanged = true;

			rttr::variant NewValue = TempValue;
			FirePropertyCommand(rInstance, rProperty, NewValue);
		}
	});

	// IMGUI ARR ENTITY Rendering
	CreateArrayPropertyLookUp<Entity*>([=](rttr::instance&, rttr::property& rProperty, rttr::variant_sequential_view& rArrView, unsigned int uiIndex, const std::string& rLabel, bool* bIsElementAndChanged)
	{
		Entity* element = rArrView.get_value(uiIndex).get_type() != rttr::type::get<nullptr_t>() ? rArrView.get_value(uiIndex).convert<Entity*>() : nullptr;

		std::string tempValueText = element ? element->GetName() : std::string("None");

		ImGui::InputText(("##EntitySource_" + rProperty.get_name().to_string()).c_str(), &tempValueText, ImGuiInputTextFlags_ReadOnly);

		if (ImGui::BeginDragDropTarget())
		{
			ImGuiStyle& style = ImGui::GetStyle();
			if (const ImGuiPayload * payload = ImGui::AcceptDragDropPayload("Entity_Drag-And-Drop", ImGuiDragDropFlags_AcceptBeforeDelivery))
			{
				// we can now control if we could actually override it.
				const auto payload_n = static_cast<Entity * *>(payload->Data);
				if (payload_n != nullptr)
				{
					// first check if we just need the base class
					// second so the thing that we want is not the base type that we need
					const auto ArrPropertyType = rArrView.get_value_type();
					const auto& infoType = (*payload_n)->get_derived_info().m_type;
					if (ArrPropertyType != rttr::type::get<Component*>() && // check if the type is entity 
						(ArrPropertyType.is_pointer() ? ArrPropertyType.get_raw_type() : ArrPropertyType) != infoType && // check if the type is the same type
						!ArrPropertyType.is_base_of((*payload_n)->get_type())) // check if the type is derived of the property type
					{
						style.Colors[ImGuiCol_DragDropTarget] = ImVec4(1.0f, 0.0f, 0.0f, 1.0f);
					}
					else
					{
						style.Colors[ImGuiCol_DragDropTarget] = ImVec4(0.0f, 1.0f, 0.0f, 1.0f);
						if (payload->IsDelivery())
						{
							rttr::variant varElement = element = *payload_n;
							if (varElement.convert(rArrView.get_value_type()) && rArrView.set_value(uiIndex, varElement))
							{
								if (bIsElementAndChanged)* bIsElementAndChanged = true;
								return;
							}

							m_pEditor->GetApplication()->GetLoggingSystem().Log(LogLevel::LOGLEVEL_WARNING, "PropertyRenderer", "Unable to replace element inside vector! : " + rLabel);
						}
					}
				}
			}

			ImGui::EndDragDropTarget();
		}
	});

	// IMGUI ARR COMPONENT POINTER Rendering
	CreateArrayPropertyLookUp<Component*>([=](rttr::instance&, rttr::property& rProperty, rttr::variant_sequential_view& rArrView, unsigned int uiIndex, const std::string& rLabel, bool* bIsElementAndChanged)
	{
		Component* element = rArrView.get_value(uiIndex).get_type() != rttr::type::get<nullptr_t>() ? rArrView.get_value(uiIndex).convert<Component*>() : nullptr;

		std::string tempValueText = element ? element->GetOwner()->GetName() + " (" + element->get_type().get_raw_type().get_name().to_string() + ")" : std::string("None (" + rArrView.get_value_type().get_raw_type().get_name().to_string() + ")");

		ImGui::InputText(("##Source_" + rProperty.get_name().to_string()).c_str(), &tempValueText, ImGuiInputTextFlags_ReadOnly);

		if (ImGui::BeginDragDropTarget())
		{
			ImGuiStyle& style = ImGui::GetStyle();
			if (const ImGuiPayload * payload = ImGui::AcceptDragDropPayload("Entity_Drag-And-Drop", ImGuiDragDropFlags_AcceptBeforeDelivery))
			{
				// we can now control if we could actually override it.
				const auto payload_n = static_cast<Entity * *>(payload->Data);
				if (payload_n != nullptr)
				{
					// let's see if the entity has this component
					rttr::type ArrPropertyType = rArrView.get_value_type();
					if (Component * pComponent = (*payload_n)->GetComponent(ArrPropertyType.get_raw_type()))
					{
						// first check if we just need the base class
						// second so the thing that we want is not the base type that we need
						const auto& infoType = pComponent->get_derived_info().m_type;
						if (ArrPropertyType != rttr::type::get<Component*>() && // check if the type is entity 
							(ArrPropertyType.is_pointer() ? ArrPropertyType.get_raw_type() : ArrPropertyType) != infoType && // check if the type is the same type
							!ArrPropertyType.is_base_of((*payload_n)->get_type())) // check if the type is derived of the property type
						{
							style.Colors[ImGuiCol_DragDropTarget] = ImVec4(1.0f, 0.0f, 0.0f, 1.0f);
						}
						else
						{
							style.Colors[ImGuiCol_DragDropTarget] = ImVec4(0.0f, 1.0f, 0.0f, 1.0f);
							if (payload->IsDelivery())
							{
								rttr::variant varComponent = pComponent;
								if (varComponent.convert(rArrView.get_value_type()) && rArrView.set_value(uiIndex, varComponent))
								{
									if (bIsElementAndChanged)* bIsElementAndChanged = true;
									return;
								}
								m_pEditor->GetApplication()->GetLoggingSystem().Log(LogLevel::LOGLEVEL_WARNING, "PropertyRenderer", "Unable to set value in type! : " + rLabel);
							}
						}
					}
				}
			}

			ImGui::EndDragDropTarget();
		}
	});

	// IMGUI VECTOR2 Rendering
	CreateArrayPropertyLookUp<Vector2>([=](rttr::instance& rInstance, rttr::property& rProperty, rttr::variant_sequential_view& rArrView, unsigned int uiIndex, const std::string& rLabel, bool* bIsElementAndChanged)
	{

		std::string strIndex = std::to_string(uiIndex);
		const Vector2 TempValue = rArrView.get_value(uiIndex).convert<Vector3>();
		float TempArrayValues[] = { TempValue.x, TempValue.y };

		if (ImGui::InputFloat2(("##Vec2_Source_" + rInstance.get_derived_type().get_raw_type().get_name().to_string() + "_" + strIndex + "_" + rProperty.get_name().to_string()).c_str(), TempArrayValues, 4, ImGuiInputTextFlags_EnterReturnsTrue))
		{
			rttr::variant NewValue = Vector2(TempArrayValues[0], TempArrayValues[1]);
			if (rArrView.set_value(uiIndex, NewValue))
			{
				if (bIsElementAndChanged)* bIsElementAndChanged = true;
				return;
			}
			m_pEditor->GetApplication()->GetLoggingSystem().Log(LogLevel::LOGLEVEL_WARNING, "PropertyRenderer", "Unable to replace element inside vector! : " + rLabel);
		}

		// We don't have to return false
		// return false;
	});

	// IMGUI VECTOR3 Rendering
	CreateArrayPropertyLookUp<Vector3>([=](rttr::instance& rInstance, rttr::property& rProperty, rttr::variant_sequential_view& rArrView, unsigned int uiIndex, const std::string& rLabel, bool* bIsElementAndChanged)
	{
		const std::string strIndex = std::to_string(uiIndex);
		const Vector3 TempValue = rArrView.get_value(uiIndex).convert<Vector3>();
		float TempArrayValues[] = { TempValue.x, TempValue.y, TempValue.z };

		if (ImGui::InputFloat3(("##Vec3_Source_" + rInstance.get_derived_type().get_raw_type().get_name().to_string() + "_" + strIndex + "_" + rProperty.get_name().to_string()).c_str(), TempArrayValues, 4, ImGuiInputTextFlags_EnterReturnsTrue))
		{
			rttr::variant NewValue = Vector3(TempArrayValues[0], TempArrayValues[1], TempArrayValues[2]);
			if (rArrView.set_value(uiIndex, NewValue))
			{
				if (bIsElementAndChanged)* bIsElementAndChanged = true;
				return;
			}
			m_pEditor->GetApplication()->GetLoggingSystem().Log(LogLevel::LOGLEVEL_WARNING, "PropertyRenderer", "Unable to replace element inside vector! : " + rLabel);
		}
	});

	// IMGUI VECTOR4 Rendering
	CreateArrayPropertyLookUp<Vector4>([=](rttr::instance& rInstance, rttr::property& rProperty, rttr::variant_sequential_view& rArrView, unsigned int uiIndex, const std::string& rLabel, bool* bIsElementAndChanged)
	{
		std::string strIndex = std::to_string(uiIndex);
		const Vector4 TempValue = rArrView.get_value(uiIndex).convert<Vector4>();
		float TempArrayValues[] = { TempValue.x, TempValue.y, TempValue.z, TempValue.w };

		if (ImGui::InputFloat4(("##Vec3_Source_" + rInstance.get_derived_type().get_raw_type().get_name().to_string() + "_" + strIndex + "_" + rProperty.get_name().to_string()).c_str(), TempArrayValues, 4, ImGuiInputTextFlags_EnterReturnsTrue))
		{
			rttr::variant NewValue = Vector4(TempArrayValues[0], TempArrayValues[1], TempArrayValues[2], TempArrayValues[3]);
			if (rArrView.set_value(uiIndex, NewValue))
			{
				if (bIsElementAndChanged)* bIsElementAndChanged = true;
				return;
			}
			m_pEditor->GetApplication()->GetLoggingSystem().Log(LogLevel::LOGLEVEL_WARNING, "PropertyRenderer", "Unable to replace element inside vector! : " + rLabel);
		}
	});

	// IMGUI ARR BOOL Rendering
	CreateArrayPropertyLookUp<bool>([=](rttr::instance& rInstance, rttr::property& rProperty, rttr::variant_sequential_view& rArrView, unsigned int uiIndex, const std::string& rLabel, bool* bIsElementAndChanged)
	{

		bool TempValue = rArrView.get_value(uiIndex).to_bool();

		if (ImGui::Checkbox(("##Bool_Source_" + rInstance.get_derived_type().get_raw_type().get_name().to_string() + "_" + std::to_string(uiIndex) + "_" + rProperty.get_name().to_string()).c_str(), &TempValue))
		{
			rttr::variant NewValue = TempValue;
			if (rArrView.set_value(uiIndex, NewValue))
			{
				if (bIsElementAndChanged)* bIsElementAndChanged = true;
				return;
			}
			m_pEditor->GetApplication()->GetLoggingSystem().Log(LogLevel::LOGLEVEL_WARNING, "PropertyRenderer", "Unable to replace element inside vector! : " + rLabel);
		}
	});

	// IMGUI ARR UINT Rendering
	CreateArrayPropertyLookUp<unsigned int>([=](rttr::instance& rInstance, rttr::property& rProperty, rttr::variant_sequential_view& rArrView, unsigned int uiIndex, const std::string& rLabel, bool* bIsElementAndChanged)
	{
		// TODO uint64_t
		unsigned int TempValue = rArrView.get_value(uiIndex).to_uint32();
		auto TempIntValue = static_cast<int>(TempValue);

		if (ImGui::InputInt(("##Uint32_Source_" + rInstance.get_derived_type().get_raw_type().get_name().to_string() + "_" + std::to_string(uiIndex) + "_" + rProperty.get_name().to_string()).c_str(), &TempIntValue, 1, 100))
		{
			if (TempIntValue < 0)
				TempIntValue = 0;
			else if (TempIntValue > USHRT_MAX)
				TempIntValue = USHRT_MAX;

			rttr::variant NewValue = static_cast<unsigned int>(TempIntValue);
			if (rArrView.set_value(uiIndex, NewValue))
			{
				if (bIsElementAndChanged)* bIsElementAndChanged = true;
				return;
			}
			m_pEditor->GetApplication()->GetLoggingSystem().Log(LogLevel::LOGLEVEL_WARNING, "PropertyRenderer", "Unable to replace element inside vector! : " + rLabel);
		}
	});

	// IMGUI ARR FLOAT Rendering
	CreateArrayPropertyLookUp<float>([=](rttr::instance& rInstance, rttr::property& rProperty, rttr::variant_sequential_view& rArrView, unsigned int uiIndex, const std::string& rLabel, bool* bIsElementAndChanged)
	{
		float TempValue = rArrView.get_value(uiIndex).to_float();

		if (ImGui::InputFloat(("##Float_Source_" + rInstance.get_derived_type().get_raw_type().get_name().to_string() + "_" + std::to_string(uiIndex) + "_" + rProperty.get_name().to_string()).c_str(), &TempValue, 1.f, 100.f, 4))
		{
			rttr::variant NewValue = TempValue;
			if (rArrView.set_value(uiIndex, NewValue))
			{
				if (bIsElementAndChanged)* bIsElementAndChanged = true;
				return;
			}
			m_pEditor->GetApplication()->GetLoggingSystem().Log(LogLevel::LOGLEVEL_WARNING, "PropertyRenderer", "Unable to replace element inside vector! : " + rLabel);
		}
	});

	// IMGUI ARR INT Rendering
	CreateArrayPropertyLookUp<int>([=](rttr::instance& rInstance, rttr::property& rProperty, rttr::variant_sequential_view& rArrView, unsigned int uiIndex, const std::string& rLabel, bool* bIsElementAndChanged)
	{
		int TempValue = rArrView.get_value(uiIndex).to_int();

		if (ImGui::InputInt(("##Int_Source_" + rInstance.get_derived_type().get_raw_type().get_name().to_string() + "_" + std::to_string(uiIndex) + "_" + rProperty.get_name().to_string()).c_str(), &TempValue, 1, 100, 4))
		{
			rttr::variant NewValue = TempValue;
			if (rArrView.set_value(uiIndex, NewValue))
			{
				if (bIsElementAndChanged)* bIsElementAndChanged = true;
				return;
			}
			m_pEditor->GetApplication()->GetLoggingSystem().Log(LogLevel::LOGLEVEL_WARNING, "PropertyRenderer", "Unable to replace element inside vector! : " + rLabel);
		}
	});

	// IMGUI ARR STRING Rendering
	CreateArrayPropertyLookUp<std::string>([=](rttr::instance& rInstance, rttr::property& rProperty, rttr::variant_sequential_view& rArrView, unsigned int uiIndex, const std::string& rLabel, bool* bIsElementAndChanged)
	{
		std::string TempValue = rArrView.get_value(uiIndex).to_string();

		if (ImGui::InputText(("##String_Source_" + rInstance.get_derived_type().get_raw_type().get_name().to_string() + "_" + std::to_string(uiIndex) + "_" + rProperty.get_name().to_string()).c_str(), &TempValue))
		{
			rttr::variant NewValue = TempValue;
			if (rArrView.set_value(uiIndex, NewValue))
			{
				if (bIsElementAndChanged)* bIsElementAndChanged = true;
				return;
			}
			m_pEditor->GetApplication()->GetLoggingSystem().Log(LogLevel::LOGLEVEL_WARNING, "PropertyRenderer", "Unable to replace element inside vector! : " + rLabel);
		}
	});

	// IMGUI ARR STRUCT/CLASS Rendering
	CreateArrayPropertyLookUp<VClass>([=](rttr::instance& rInstance, rttr::property& rProperty, rttr::variant_sequential_view& rArrView, unsigned int uiIndex, const std::string& rLabel, bool* bIsElementAndChanged)
	{
		// Remark that is can be a wrapper sometimes, be careful on the other array properties

		// Grab the actual TSubClass or struct, class
		rttr::variant TempValueVar = (rArrView.get_value(uiIndex).get_type().is_wrapper()) ? rArrView.get_value(uiIndex).extract_wrapped_value() : rArrView.get_value(uiIndex);
		// Grab the base class to grab the inner class
		VClass TempValue = TempValueVar.get_value<VClass>();

		// If it is a TSubclass do something different
		if (TempValue.is_sub_class())
		{
			// Subclass ID title
			const std::string SubclassSetter = "SubclassSetter";
			const ImGuiInputTextFlags PropertyFlags = ImGuiInputTextFlags_ReadOnly;

			const bool PopUpOpen = ImGui::IsPopupOpen(SubclassSetter.c_str());

			const auto derived_classes = TempValueVar.get_type().invoke("GetDerivedTypes", TempValueVar, {}).get_value<rttr::array_range<rttr::type>>();

			// If we have the base selected then render none.
			if (ImGui::Selectable(((TempValueVar.get_type().invoke("IsSame", TempValueVar, {}).to_bool() ? TempValue.get_inner_type().get_name().to_string() : TempValue.get_inner_type().get_name().to_string()) + "###" + rLabel).c_str()))
			{
				if (!PopUpOpen)
				{
					ImGui::OpenPopup(SubclassSetter.c_str());
					m_PopUpSearchString.clear();
				}
			}

			// None make a big dropdown list
			uint32_t uiDropdownIndex = 0;

			ImList<std::string> SubClassDropdown("TSubclass_Source_" + TempValue.get_type().get_name().to_string() + "_Source_Type_", PropertyFlags);

			int counter = 0;
			for (const auto& derived_class : derived_classes)
			{
				SubClassDropdown.AddValue(derived_class.get_raw_type().get_name().to_string(), derived_class.get_name().to_string());
				counter++;

#ifdef EDITOR
				if (TempValue.m_InstantiateType == derived_class.get_name().to_string())
					uiDropdownIndex = counter;
#endif
			}

			// if it is empty we don't have derived classes.
			if (derived_classes.empty())
			{
				uiDropdownIndex = ++counter;
				SubClassDropdown.AddValue(TempValue.get_inner_type().get_raw_type().get_name().to_string(), TempValue.get_inner_type().get_name().to_string());
			}

			const ImVec2 SelectableSize = ImGui::GetItemRectSize();
			ImGui::SetNextWindowSize(ImVec2(SelectableSize.x, SelectableSize.y * 10));

			if (ImGui::BeginPopup(SubclassSetter.c_str()))
			{
				ImGui::PushItemWidth(SelectableSize.x);

				if (ImGui::IsRootWindowOrAnyChildFocused() && !ImGui::IsAnyItemActive() && !ImGui::IsMouseClicked(0))
					ImGui::SetKeyboardFocusHere(0);

				// Set the search value
				SubClassDropdown.SetSearchValue(m_PopUpSearchString.c_str());
				SubClassDropdown.SetCurrentValue(uiDropdownIndex);
				SubClassDropdown.AddListener([&](const std::string&, const std::string& selectedType)
				{
					rttr::type NewSelectedType = rttr::type::get_by_name(selectedType);
#ifdef EDITOR
					if (TempValue.m_InstantiateType == NewSelectedType.get_name().to_string())
					{
						ImGui::CloseCurrentPopup();
						return;
					}
#else
					if (rttr::type::get_by_name(selectedType).get_name().to_string().empty())
						return;
#endif
					const rttr::instance instValue = TempValueVar;
					TempValueVar.get_type().invoke("SetInstantiateType", instValue, { NewSelectedType });

					// TODO test if this works!
					FirePropertyCommand(rInstance, rProperty, TempValueVar);

					ImGui::CloseCurrentPopup();
				});

				SubClassDropdown.Draw();

				// after the draw we need to grab the search value
				m_PopUpSearchString = SubClassDropdown.GetSearchValue();

				ImGui::EndPopup();
			}
		}
		else
		{ // This is not a TSubclass then render it like normal class/struct

			rttr::instance vClassInstance = TempValueVar;
			for (auto prop : vClassInstance.get_derived_type().get_properties())
			{
				const std::string elPropName = "##_" + std::to_string(uiIndex) + "_" + prop.get_name().to_string() + "_";
				Render(vClassInstance, prop, elPropName, bIsElementAndChanged);
			}

			bool b = TempValueVar.convert(rArrView.get_value_type());
			if (!rArrView.set_value(uiIndex, TempValueVar))
			{
				m_pEditor->GetApplication()->GetLoggingSystem().Log(LogLevel::LOGLEVEL_WARNING, "PropertyRenderer", "Unable to replace element inside vector! : " + rLabel);
				if (bIsElementAndChanged)* bIsElementAndChanged = false;
				// return false;
			}
		}
	});

	// IMGUI KEY INT MAP Rendering
	CreateKeyMapPropertyLookUp<int>([](rttr::instance& rInstance, rttr::property& rProperty, rttr::variant_associative_view& rArrView, rttr::variant& rKeyItem, int32_t iIndex, const std::string&)
	{
		std::pair<rttr::variant, bool> pair = { rKeyItem, false };
		int TempValue = (rKeyItem.get_type() != rttr::type::get<nullptr_t>()) ? rKeyItem.get_value<int>() : 0;

		if (ImGui::InputInt(("##Int_Source_" + std::to_string(iIndex) + "_" + rInstance.get_derived_type().get_raw_type().get_name().to_string() + "_" + rProperty.get_name().to_string()).c_str(), &TempValue, 1, 100, ImGuiInputTextFlags_EnterReturnsTrue))
		{
			pair.first = TempValue;
			pair.second = true;
		}

		return pair;
	});

	// IMGUI KEY STRING MAP Rendering
	CreateKeyMapPropertyLookUp<std::string>([](rttr::instance& rInstance, rttr::property& rProperty, rttr::variant_associative_view& rArrView, rttr::variant& rKeyItem, int32_t iIndex, const std::string&)
	{
		std::pair<rttr::variant, bool> pair = { rKeyItem, false };
		std::string TempValue = (rKeyItem.get_type() != rttr::type::get<nullptr_t>()) ? rKeyItem.get_value<std::string>() : std::string("");

		if (ImGui::InputText(("##String_Key_Source_" + std::to_string(iIndex) + "_" + rInstance.get_derived_type().get_raw_type().get_name().to_string() + "_" + rProperty.get_name().to_string()).c_str(), &TempValue, ImGuiInputTextFlags_EnterReturnsTrue))
		{
			rttr::variant NewValue = TempValue;
			pair.first = NewValue;
			pair.second = true;
		}

		return pair;
	});

	// IMGUI VALUE STRING MAP Rendering
	CreateValueMapPropertyLookUp<std::string>([](rttr::instance& rInstance, rttr::property& rProperty, rttr::variant_associative_view& rArrView, rttr::variant& rKeyItem, rttr::variant& rValueItem, int32_t iIndex, const std::string&)
	{
		std::pair<rttr::variant, bool> pair = { rValueItem, false };

		std::string TempValue = (rValueItem.get_type() != rttr::type::get<nullptr_t>()) ? rValueItem.get_value<std::string>() : std::string("");

		if (ImGui::InputText(("##String_Value_Source_" + std::to_string(iIndex) + rInstance.get_derived_type().get_raw_type().get_name().to_string() + "_" + rProperty.get_name().to_string()).c_str(), &TempValue, ImGuiInputTextFlags_EnterReturnsTrue))
		{
			rttr::variant NewValue = TempValue;
			if (iIndex == -1)
			{
				pair.first = NewValue;
				pair.second = true;
			}
			else
			{
				auto itr = rArrView.find(rKeyItem);
				if (itr != rArrView.end())
				{
					auto& sNewStringValue = const_cast<std::string&>(itr.get_value().get_wrapped_value<std::string>());
					sNewStringValue = TempValue.c_str();

					pair.first = sNewStringValue;
					pair.second = true;
				}
			}
		}

		return pair;
	});

}

void PropertyRenderer::InitializeResourcePropertyLookUp()
{
	// IMGUI .vox resource rendering
	CreateDefaultResourcePropertyLookUp("vox");

	// IMGUI .Ogg resource rendering
	CreateDefaultResourcePropertyLookUp("ogg");

	// IMGUI .Ogg resource rendering
	CreateDefaultResourcePropertyLookUp("png");

	// IMGUI .Vox resource rendering
	CreateDefaultResourcePropertyLookUp("anim.vox");

	// IMGUI .Wld resource rendering
	CreateDefaultResourcePropertyLookUp("wld");
}

void PropertyRenderer::RenderProperty(rttr::instance& rInstance, rttr::property& rProperty, std::string& Label, bool* bIsElementAndChanged)
{
	std::unordered_map<rttr::type, std::function<void(rttr::instance&, rttr::property&, std::string&, bool*)>>::iterator Found = m_PropertyLookUp.find(rProperty.get_type());

	if (Found != m_PropertyLookUp.end())
	{
		m_PropertyLookUp[rProperty.get_type()](rInstance, rProperty, Label, bIsElementAndChanged);
	}
	else
	{
		const auto EntityType = rttr::type::get<Entity*>();
		const auto ComponentType = rttr::type::get<Component*>();
		const auto VClassType = rttr::type::get<VClass>();

		if (EntityType.is_base_of(rProperty.get_type()))
		{
			// see if it is an entity
			m_PropertyLookUp[rProperty.get_type().is_pointer() ? EntityType : rttr::type::get<Entity>()](rInstance, rProperty, Label, bIsElementAndChanged);
			return;
		}

		// now check for the components
		if (ComponentType.is_base_of(rProperty.get_type()))
		{
			// see if it is an component
			m_PropertyLookUp[rProperty.get_type().is_pointer() ? ComponentType : rttr::type::get<Component>()](rInstance, rProperty, Label, bIsElementAndChanged);
			return;
		}

		if (VClassType.is_base_of(rProperty.get_type()))
		{	
			m_PropertyLookUp[VClassType](rInstance, rProperty, Label, bIsElementAndChanged);
			return;
		}

		// Now check for array types
		if (rProperty.get_type().is_sequential_container())
		{
			RenderArrayProperty(rInstance, rProperty, Label);
			return;
		}

		// Support for map types
		if (rProperty.get_type().is_associative_container())
		{
			RenderMapProperty(rInstance, rProperty, Label);
			return;
		}

		m_PropertyLookUp[rProperty.get_type()] = m_NotSupportedProperty;
		m_pEditor->GetApplication()->GetLoggingSystem().Log(LogLevel::LOGLEVEL_ERROR, "PropertyRenderer", "Property type to read, write and render not supported! (" + rProperty.get_type().get_name().to_string() + ")");
	}
}

void PropertyRenderer::RenderEnumProperty(rttr::instance& rObject, rttr::property& rProperty, std::string& Label, bool* bIsElementAndChanged)
{
	ImGuiInputTextFlags PropertyFlags = ImGuiInputTextFlags_CallbackAlways;

	if (rProperty.is_readonly())
		PropertyFlags = PropertyFlags | ImGuiColorEditFlags_NoInputs;

	rttr::variant TempValue = rProperty.get_value(rObject);

	rttr::enumeration EnumValue = TempValue.get_type().get_enumeration();

	auto listEnums = EnumValue.get_values();

	std::vector<rttr::variant> EnumTypeSelections;
	EnumTypeSelections.insert(EnumTypeSelections.end(), std::make_move_iterator(listEnums.begin()), std::make_move_iterator(listEnums.end()));

	uint32_t uiIndex = 0;
	ImList<rttr::variant> MyEnumDropDown(
		"Enum_" + Label + EnumValue.get_type().get_name().to_string() + "_Source_Type_",
		EnumTypeSelections,
		[TempValue, &uiIndex](uint32_t uiCurrentIndex, const rttr::variant& EnumType)
	{
		const std::string NameValueEnum = EnumType.to_string();

		if (TempValue.to_string() == NameValueEnum)
			uiIndex = uiCurrentIndex;

		return NameValueEnum;
	},
		PropertyFlags
		);

	MyEnumDropDown.SetRenderType(LT_DROPDOWN);
	MyEnumDropDown.SetCurrentValue(uiIndex);
	MyEnumDropDown.AddListener([&](const std::string&, const rttr::variant& selectedType)
	{
		TempValue = selectedType;
		if (rProperty.set_value(rObject, TempValue))
		{
			if (bIsElementAndChanged)* bIsElementAndChanged = true;
			return;
		}
		m_pEditor->GetApplication()->GetLoggingSystem().Log(LogLevel::LOGLEVEL_WARNING, "PropertyRenderer", "Unable to set value in type! : " + Label);
	});

	MyEnumDropDown.Draw();
}

void PropertyRenderer::RenderArrayProperty(rttr::instance& rInstance, rttr::property& rProperty, std::string& Label)
{
	// init
	rttr::variant NewValue = rProperty.get_value(rInstance);
	rttr::variant_sequential_view arrView = NewValue.create_sequential_view();

	// Base types
	const auto EntityType = rttr::type::get<Entity*>();
	const auto ComponentType = rttr::type::get<Component*>();
	const auto VClassType = rttr::type::get<VClass>();

	// first check if it a normal type
	rttr::type rRenderType = arrView.get_value_type();
	auto ArrayPropertyFound = m_ArrayPropertyLookUp.find(rRenderType);

	// maybe we need a base class check for it.
	if (ArrayPropertyFound == m_ArrayPropertyLookUp.end())
	{
		if (Utils::CheckArrayDerivedType(rProperty, EntityType))
		{
			rRenderType = EntityType;
			ArrayPropertyFound = m_ArrayPropertyLookUp.find(rRenderType);
		}

		if (Utils::CheckArrayDerivedType(rProperty, ComponentType))
		{
			rRenderType = ComponentType;
			ArrayPropertyFound = m_ArrayPropertyLookUp.find(rRenderType);
		}

		if (Utils::CheckArrayDerivedType(rProperty, VClassType))
		{
			rRenderType = VClassType;
			ArrayPropertyFound = m_ArrayPropertyLookUp.find(rRenderType);
		}
	}

	if (ArrayPropertyFound != m_ArrayPropertyLookUp.end())
	{
		bool bIsChanged = false;
		const auto PropertyFlag = (!arrView.is_dynamic()) ? ImGuiInputTextFlags_ReadOnly : 0;

		auto ContainerSize = static_cast<int>(arrView.get_size());
		auto CurrentSize = ContainerSize;

		ImGui::NewLine();
		if (ImGui::InputInt(("##Size_" + rInstance.get_derived_type().get_raw_type().get_name().to_string() + "_" + arrView.get_value_type().get_name().to_string() + "_" + rProperty.get_name().to_string()).c_str(), &CurrentSize, 1, 100, PropertyFlag))
		{
			bIsChanged = true;
			// check if the container is dynamic
			if (arrView.is_dynamic())
			{
				// vector check
				if (CurrentSize < 0)
				{
					CurrentSize = 0;

					// Only clear if we have elements
					if (!arrView.is_empty())
						arrView.clear();
				}

				else if (CurrentSize > USHRT_MAX)
					CurrentSize = USHRT_MAX;

				if (CurrentSize > ContainerSize)
				{
					// increase current size
					const auto rArrType = arrView.get_value_type();
					if (rArrType.is_pointer())
					{
						arrView.insert(arrView.end(), nullptr);
					} else
					{
						arrView.insert(arrView.end(), (rArrType.is_wrapper()) ? rArrType.get_wrapped_type().create() : rArrType.create());
					}


					if (!arrView.set_size(CurrentSize))
					{
						m_pEditor->GetApplication()->GetLoggingSystem().Log(LogLevel::LOGLEVEL_WARNING, "PropertyRenderer", "Unable to increase the vector! : " + Label);
					}

					ContainerSize = CurrentSize;
				}
				else
				{
					if (ContainerSize > 0)
					{
						// we are decreasing
						const SizeType CurrentIndex = ContainerSize - 1;
						arrView.erase(arrView.begin() + CurrentIndex);
						ContainerSize = CurrentSize;
					}
				}
			}
		}

		if (!arrView.is_empty())
		{
			const bool isVClass = rRenderType == VClassType;
			const float customSize = (isVClass) ? arrView.get_value_type().get_properties().size() * 10.0f * arrView.get_size() : 0.0f;

			// ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(50.0f, 50.0f));
			ImGui::PushStyleColor(ImGuiCol_ChildWindowBg, ImVec4(0.f, 0.f, 0.f, 0.f));
			ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));
			ImGui::BeginChild(("##ScrollingRegion_" + rInstance.get_derived_type().get_raw_type().get_name().to_string() + "_" + arrView.get_value_type().get_name().to_string() + "_" + rProperty.get_name().to_string()).c_str(), ImVec2(0.0f, customSize + ContainerSize * 20.0f), false);
			ImGui::Columns(2, ("ContentColumns_" + rInstance.get_derived_type().get_raw_type().get_name().to_string() + "_" + arrView.get_value_type().get_name().to_string() + "_" + rProperty.get_name().to_string()).c_str(), false);
			ImGui::SetColumnWidth(0, 20.0f);
			ImGui::SetColumnWidth(1, ImGui::GetWindowSize().x);

			for (unsigned int i = 0; i < static_cast<unsigned int>(ContainerSize); ++i)
			{
				const std::string& strIndex = std::to_string(i);

				ImGui::Text("%s", strIndex.c_str());
				ImGui::NextColumn();

				// Render if we have a VClass 
				if (isVClass && ImGui::CollapsingHeader((strIndex + "##").c_str()))
				{

					rttr::type arrType = arrView.get_value(i).get_type();

					m_ArrayPropertyLookUp[rRenderType](rInstance, rProperty, arrView, i, Label, &bIsChanged);

				}
				// Only render normal array if we are not a vclass and not closed
				else if (!isVClass) m_ArrayPropertyLookUp[rRenderType](rInstance, rProperty, arrView, i, Label, &bIsChanged);
				ImGui::NextColumn();

			}

			ImGui::Columns(1);
			ImGui::EndChild();
			ImGui::PopStyleVar();
			ImGui::PopStyleColor();
			// ImGui::PopStyleVar();
		}

		if (bIsChanged)
		{
			FirePropertyCommand(rInstance, rProperty, NewValue);
			// m_pEditor->GetApplication()->GetLoggingSystem().Log(LogLevel::LOGLEVEL_WARNING, "PropertyRenderer", "Unable to set value in type! : " + Label);
		}
	}
}

void PropertyRenderer::RenderMapProperty(rttr::instance& rInstance, rttr::property& rProperty, std::string& Label)
{
	// init
	rttr::variant var = rProperty.get_value(rInstance);
	rttr::variant_associative_view arrView = var.create_associative_view();

	// Base types
	const auto EntityType = rttr::type::get<Entity*>();
	const auto ComponentType = rttr::type::get<Component*>();

	// first check if it a normal type
	rttr::type rKeyType = arrView.get_key_type();
	rttr::type rValueType = arrView.get_value_type();
	auto KeyMapPropertyFound = m_KeyMapPropertyLookUp.find(rKeyType);
	auto ValueMapPropertyFound = m_ValueMapPropertyLookUp.find(rValueType);

	// maybe we need a base class check for it.
	if (KeyMapPropertyFound == m_KeyMapPropertyLookUp.end())
	{
		// Let see if the key type has a base lookup
		if (Utils::CheckArrayDerivedType(rProperty, EntityType))
		{
			rKeyType = EntityType;
			KeyMapPropertyFound = m_KeyMapPropertyLookUp.find(rKeyType);
		}

		if (Utils::CheckArrayDerivedType(rProperty, ComponentType))
		{
			rKeyType = ComponentType;
			KeyMapPropertyFound = m_KeyMapPropertyLookUp.find(rKeyType);
		}
	}

	// maybe we need a base class check for it.
	if (ValueMapPropertyFound == m_ValueMapPropertyLookUp.end())
	{
		// Let see if the key type has a base lookup
		if (Utils::CheckArrayDerivedType(rProperty, EntityType))
		{
			rValueType = EntityType;
			ValueMapPropertyFound = m_ValueMapPropertyLookUp.find(rValueType);
		}

		if (Utils::CheckArrayDerivedType(rProperty, ComponentType))
		{
			rValueType = ComponentType;
			ValueMapPropertyFound = m_ValueMapPropertyLookUp.find(rValueType);
		}
	}

	// Rendering of the key values
	if (KeyMapPropertyFound != m_KeyMapPropertyLookUp.end() || ValueMapPropertyFound != m_ValueMapPropertyLookUp.end())
	{
		bool bIsChanged = false;

		// Create reference map
		TMap Map = { arrView };

		const auto KeyValuePairItr = m_PlaceholderMapValues.find(rProperty.get_name().to_string());
		std::pair<rttr::variant, rttr::variant> PlaceholderKeyValuePair = { nullptr, nullptr };
		// we found the already created pair
		if (KeyValuePairItr != m_PlaceholderMapValues.end())
		{
			PlaceholderKeyValuePair = KeyValuePairItr->second;
		}

		const auto ContainerSize = static_cast<int>(arrView.get_size());

		ImGui::NewLine();

		// Let us make the add button with two empty values
		ImGui::PushStyleColor(ImGuiCol_ChildWindowBg, ImVec4(0.f, 0.f, 0.f, 0.f));

		ImGui::BeginChild(("##ScrollingRegion_" + rInstance.get_derived_type().get_raw_type().get_name().to_string() + "_" + arrView.get_value_type().get_name().to_string() + "_" + rProperty.get_name().to_string()).c_str(), ImVec2(0, ContainerSize * 30.0f + 60.0f), false);
		ImGui::Columns(3, ("ContentColumns_" + rInstance.get_derived_type().get_raw_type().get_name().to_string() + "_" + arrView.get_value_type().get_name().to_string() + "_" + rProperty.get_name().to_string()).c_str(), false);

		ImGui::Text("%s", std::string("Key ").data());
		ImGui::NextColumn();

		ImGui::Text("%s", std::string("Value ").data());
		ImGui::NextColumn();
		ImGui::NextColumn();

		auto KeyPair = m_KeyMapPropertyLookUp[rKeyType](rInstance, rProperty, arrView, PlaceholderKeyValuePair.first, -1, Label);
		ImGui::NextColumn();

		rttr::variant rNullptrVariant = rttr::variant(nullptr);
		auto ValuePair = m_ValueMapPropertyLookUp[rValueType](rInstance, rProperty, arrView, rNullptrVariant, PlaceholderKeyValuePair.second, -1, Label);
		ImGui::NextColumn();
		const bool bPressed = ImGui::Button(("Add ##Size_" + rInstance.get_derived_type().get_raw_type().get_name().to_string() + "_" + arrView.get_value_type().get_name().to_string() + "_" + rProperty.get_name().to_string()).c_str());
		ImGui::NextColumn();

		// TODO make batch insert

		// if we have made a key and value
		if (KeyPair.second || ValuePair.second)
		{
			const auto& tKeyType = arrView.get_key_type();
			KeyPair.first.convert(tKeyType);

			const auto& tValueType = arrView.get_value_type();
			ValuePair.first.convert(tValueType);

			PlaceholderKeyValuePair = { KeyPair.first, ValuePair.first };

			if (KeyValuePairItr == m_PlaceholderMapValues.end())
			{	// if it does not exist add it to the map
				m_PlaceholderMapValues.insert(std::make_pair(rProperty.get_name(), PlaceholderKeyValuePair));
			}
			else
			{
				KeyValuePairItr->second = PlaceholderKeyValuePair;
			}
		}

		if (bPressed)
		{
			// Let's see if we can insert
			const auto& itr = arrView.find(PlaceholderKeyValuePair.first);

			if (itr == arrView.end())
			{
				// TODO convert the variants inside the PlaceholderKeyValuePair. or else they will not be added to the map

				if (Map.Insert(arrView, PlaceholderKeyValuePair.first, PlaceholderKeyValuePair.second))
				{
					bIsChanged = true;
				}
			}
			else
			{
				m_pEditor->GetApplication()->GetLoggingSystem().Log(LogLevel::LOGLEVEL_WARNING, "PropertyRenderer", "Unable to set value in map! : " + Label);
			}
		}

		// Key, Value
		std::pair<rttr::variant, rttr::variant> ChangedKeyPair = { nullptr, nullptr };
		std::pair<rttr::variant, rttr::variant> ChangedValuePair = { nullptr, nullptr };

		if (!arrView.is_empty())
		{
			int32_t counter = 0;
			for (auto& item : arrView)
			{
				rttr::variant rKey = item.first.extract_wrapped_value();
				rttr::variant rValue = item.second.extract_wrapped_value();

				// Render the key values
				const auto& arrKeyPair = m_KeyMapPropertyLookUp[rKeyType](rInstance, rProperty, arrView, rKey, counter, Label);
				if (arrKeyPair.second)
					ChangedKeyPair = { rKey, arrKeyPair.first };

				ImGui::NextColumn();

				const auto& arrValuePair = m_ValueMapPropertyLookUp[rValueType](rInstance, rProperty, arrView, rKey, rValue, counter, Label);
				if (arrValuePair.second)
					bIsChanged = true;

				ImGui::NextColumn();
				ImGui::NextColumn();

				counter++;
			}

			// First let us make the change key algorithm
			if (ChangedKeyPair.second.get_type() != rttr::type::get<nullptr_t>())
			{
				bIsChanged = Map.UpdateKey(arrView, ChangedKeyPair.first, ChangedKeyPair.second);
			}

			if (ChangedValuePair.second.get_type() != rttr::type::get<nullptr_t>())
			{
				bIsChanged = Map.UpdateValue(arrView, ChangedValuePair.first, ChangedValuePair.second);
			}
		}

		ImGui::EndChild();
		ImGui::PopStyleColor();

		// Fire property command
		if (bIsChanged)
		{
			FirePropertyCommand(rInstance, rProperty, var);
			m_PlaceholderMapValues.erase(rProperty.get_name().to_string());
		}
	}
}

void PropertyRenderer::RenderResource(const std::string& ResourceExtension, rttr::instance & rInstance, rttr::property & rProperty, std::string Label, bool* bIsElementAndChanged)
{
	std::unordered_map<std::string, std::function<void(rttr::instance&, rttr::property&, std::string&, bool*)>>::iterator Found = m_ResourcePropertyLookUp.find(ResourceExtension);

	if (Found != m_ResourcePropertyLookUp.end())
	{
		m_ResourcePropertyLookUp[ResourceExtension](rInstance, rProperty, Label, bIsElementAndChanged);
	}
	else
	{
		m_ResourcePropertyLookUp[ResourceExtension] = m_NotSupportedProperty;

		m_pEditor->GetApplication()->GetLoggingSystem().Log(LogLevel::LOGLEVEL_ERROR, "PropertyRenderer", "Resource type to read, write and render not supported! (" + rProperty.get_type().get_name().to_string() + ")");
	}
}

void PropertyRenderer::RenderArrayResource(const std::string & ResourceExtension, rttr::instance & rInstance, rttr::property & rProperty, std::string & Label)
{
	// init
	rttr::variant NewValue = rProperty.get_value(rInstance);
	rttr::variant_sequential_view arrView = NewValue.create_sequential_view();

	// first check if it a normal type
	rttr::type rRenderType = arrView.get_value_type();
	auto ArrayPropertyFound = m_ArrayPropertyLookUp.find(rRenderType);

	if (ArrayPropertyFound != m_ArrayPropertyLookUp.end())
	{
		bool bIsChanged = false;
		const auto PropertyFlag = (!arrView.is_dynamic()) ? ImGuiInputTextFlags_ReadOnly : 0;

		auto ContainerSize = static_cast<int>(arrView.get_size());
		auto CurrentSize = ContainerSize;

		ImGui::NewLine();
		if (ImGui::InputInt(("##Size_" + rInstance.get_derived_type().get_raw_type().get_name().to_string() + "_" + arrView.get_value_type().get_name().to_string() + "_" + rProperty.get_name().to_string()).c_str(), &CurrentSize, 1, 100, PropertyFlag))
		{
			bIsChanged = true;
			// check if the container is dynamic
			if (arrView.is_dynamic())
			{
				// vector check
				if (CurrentSize < 0)
				{
					CurrentSize = 0;

					// Only clear if we have elements
					if (!arrView.is_empty())
						arrView.clear();
				}

				else if (CurrentSize > USHRT_MAX)
					CurrentSize = USHRT_MAX;

				if (CurrentSize > ContainerSize)
				{
					// increase current size
					const auto rArrType = arrView.get_value_type();
					if (rArrType.is_pointer())
					{
						arrView.insert(arrView.end(), nullptr);
					}
					else
					{
						arrView.insert(arrView.end(), (rArrType.is_wrapper()) ? rArrType.get_wrapped_type().create() : rArrType.create());
					}


					if (!arrView.set_size(CurrentSize))
					{
						m_pEditor->GetApplication()->GetLoggingSystem().Log(LogLevel::LOGLEVEL_WARNING, "PropertyRenderer", "Unable to increase the vector! : " + Label);
					}

					ContainerSize = CurrentSize;
				}
				else
				{
					if (ContainerSize > 0)
					{
						// we are decreasing
						const SizeType CurrentIndex = ContainerSize - 1;
						arrView.erase(arrView.begin() + CurrentIndex);
						ContainerSize = CurrentSize;
					}
				}
			}
		}

		if (!arrView.is_empty())
		{
			// ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(50.0f, 50.0f));
			ImGui::PushStyleColor(ImGuiCol_ChildWindowBg, ImVec4(0.f, 0.f, 0.f, 0.f));
			ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));
			ImGui::BeginChild(("##ScrollingRegion_" + rInstance.get_derived_type().get_raw_type().get_name().to_string() + "_" + arrView.get_value_type().get_name().to_string() + "_" + rProperty.get_name().to_string()).c_str(), ImVec2(0.0f, ContainerSize * 20.0f), false);
			ImGui::Columns(2, ("ContentColumns_" + rInstance.get_derived_type().get_raw_type().get_name().to_string() + "_" + arrView.get_value_type().get_name().to_string() + "_" + rProperty.get_name().to_string()).c_str(), false);
			ImGui::SetColumnWidth(0, 20.0f);
			ImGui::SetColumnWidth(1, ImGui::GetWindowSize().x);

			for (unsigned int i = 0; i < static_cast<unsigned int>(ContainerSize); ++i)
			{
				const std::string& strIndex = std::to_string(i);

				ImGui::Text("%s", strIndex.c_str());
				ImGui::NextColumn();

				rttr::type arrType = arrView.get_value(i).get_type();

				const bool arrElementChanged = m_ArrayResourcePropertyLookUp[ResourceExtension](rInstance, rProperty, arrView, i, Label);

				bIsChanged = bIsChanged || arrElementChanged;
				ImGui::NextColumn();
			}

			ImGui::Columns(1);
			ImGui::EndChild();
			ImGui::PopStyleVar();
			ImGui::PopStyleColor();
			// ImGui::PopStyleVar();
		}

		if (bIsChanged)
		{
			FirePropertyCommand(rInstance, rProperty, NewValue);
			// m_pEditor->GetApplication()->GetLoggingSystem().Log(LogLevel::LOGLEVEL_WARNING, "PropertyRenderer", "Unable to set value in type! : " + Label);
		}
	}
}

void PropertyRenderer::CreateDefaultResourcePropertyLookUp(const std::string & ResourceExtension)
{
	CreateResourcePropertyLookUp(ResourceExtension, [this, ResourceExtension](rttr::instance& rInstance, rttr::property & rProperty, std::string& rLabel, bool* bIsElementAndChanged)
	{
		// Resource title
		const std::string ResourceSetter = ResourceExtension + "Setter" + rLabel;

		const ImGuiID PopUpID = ImGui::GetID(ResourceSetter.c_str());
		const bool PopUpOpen = ImGui::IsPopupOpen(ResourceSetter.c_str());

		const std::string filename = GetResourceFileName(rProperty.get_value(rInstance).to_string());

		if (ImGui::Selectable(std::string(filename + "###_" + rLabel + rInstance.get_derived_type().get_name().to_string() + rProperty.get_value(rInstance).to_string()).c_str()))
		{
			if (!PopUpOpen)
			{
				ImGui::OpenPopup(ResourceSetter.c_str());
				m_PopUpSearchString.clear();
			}
		}
		if (ImGui::IsItemHovered() && !PopUpOpen && !filename.empty())
		{
			ImGui::BeginTooltip();
			{
				ImGui::Text("%s", filename.c_str());
			}
			ImGui::EndTooltip();
		}

		const ImVec2 SelectableSize = ImGui::GetItemRectSize();
		ImGui::SetNextWindowSize(ImVec2(SelectableSize.x * 2.5f, SelectableSize.y * 10.0f));

		if (ImGui::BeginPopup(ResourceSetter.c_str()))
		{
			ImGui::PushItemWidth(SelectableSize.x);

			if (ImGui::IsRootWindowOrAnyChildFocused() && !ImGui::IsAnyItemActive() && !ImGui::IsMouseClicked(0))
				ImGui::SetKeyboardFocusHere(0);

			// list of string values
			std::vector<std::string> VoxResourceFilePaths;

			// Grab and fill the list
			m_pEditor->GetApplication()->GetResourceManager().GetResourceFilePaths(ResourceExtension, VoxResourceFilePaths);

			ImList<std::string> imGuiList(
				ResourceSetter, // Title of the list 
				VoxResourceFilePaths, // Values
				[=](uint32_t uiIndex, const std::string& VoxResource)
			{
				return GetResourceFileName(VoxResource);
			},  // Bind function what needs to be done to get the title
				0 // No ImGui flags
			);

			// Set the search value
			imGuiList.SetSearchValue(m_PopUpSearchString.c_str());

			// Set the callback when a value is being pressed
			imGuiList.AddListener([this, &rInstance, &rProperty, bIsElementAndChanged](const std::string& TempResourceFileName, const std::string& value)
			{
				if (bIsElementAndChanged) *bIsElementAndChanged = true;
				rttr::variant NewValue = (TempResourceFileName == "None") ? "" : value;
				FirePropertyCommand(rInstance, rProperty, NewValue);
				ImGui::CloseCurrentPopup();
			});

			imGuiList.Draw();

			// after the draw we need to grab the search value
			m_PopUpSearchString = imGuiList.GetSearchValue();

			ImGui::EndPopup();
		}
	}
	);

	CreateArrayResourcePropertyLookUp(ResourceExtension, [this, ResourceExtension](rttr::instance& rInstance, rttr::property& rProperty, rttr::variant_sequential_view& rArrView, unsigned int uiIndex, const std::string& rLabel)
	{
		// Resource title
		std::string ResourceSetter = "###_" + ResourceExtension + "Setter" + rInstance.get_derived_type().get_name().to_string() + rProperty.get_value(rInstance).to_string() + "_ArrayIndex" + std::to_string(uiIndex);

		const ImGuiID PopUpID = ImGui::GetID(ResourceSetter.c_str());
		const bool PopUpOpen = ImGui::IsPopupOpen(ResourceSetter.c_str());

		const std::string filename = GetResourceFileName(rArrView.get_value(uiIndex).to_string());
		std::string propertyLabel = filename + "###_" + rLabel + rInstance.get_derived_type().get_name().to_string() + rProperty.get_value(rInstance).to_string() + "_ArrayIndex" + std::to_string(uiIndex);

		if (ImGui::Selectable(propertyLabel.c_str()))
		{
			if (!PopUpOpen)
			{
				ImGui::OpenPopup(ResourceSetter.c_str());
				m_PopUpSearchString.clear();
			}
		}
		if (ImGui::IsItemHovered() && !PopUpOpen && filename.size() > 0)
		{
			ImGui::BeginTooltip();
			{
				ImGui::Text("%s", filename.c_str());
			}
			ImGui::EndTooltip();
		}

		const ImVec2 SelectableSize = ImGui::GetItemRectSize();
		ImGui::SetNextWindowSize(ImVec2(SelectableSize.x * 2.5f, SelectableSize.y * 10.0f));
		bool ValueChanged = false;

		if (ImGui::BeginPopup(ResourceSetter.c_str()))
		{
			ImGui::PushItemWidth(SelectableSize.x);

			if (ImGui::IsRootWindowOrAnyChildFocused() && !ImGui::IsAnyItemActive() && !ImGui::IsMouseClicked(0))
				ImGui::SetKeyboardFocusHere(0);

			// list of string values
			std::vector<std::string> VoxResourceFilePaths;

			// Grab and fill the list
			m_pEditor->GetApplication()->GetResourceManager().GetResourceFilePaths(ResourceExtension, VoxResourceFilePaths);

			ImList<std::string> imGuiList(
				ResourceSetter, // Title of the list 
				VoxResourceFilePaths, // Values
				[=](uint32_t, const std::string& VoxResource) { return GetResourceFileName(VoxResource); },  // Bind function what needs to be done to get the title
				0 // No ImGui flags
			);

			// Set the search value
			imGuiList.SetSearchValue(m_PopUpSearchString.c_str());
			rttr::variant NewValue;

			// Set the callback when a value is being pressed
			imGuiList.AddListener([this, &rArrView , &uiIndex, &rInstance, &rProperty, &ValueChanged, &NewValue](const std::string& TempResourceFileName, const std::string& value)
			{
				NewValue = (TempResourceFileName == "None") ? "" : value;
				ValueChanged = rArrView.set_value(uiIndex, value);
				ImGui::CloseCurrentPopup();
			});

			imGuiList.Draw();

			// after the draw we need to grab the search value
			m_PopUpSearchString = imGuiList.GetSearchValue();

			ImGui::EndPopup();
		}

		return ValueChanged;
	}
	);
}

void PropertyRenderer::CreateResourcePropertyLookUp(const std::string& FileExtension, std::function<void(rttr::instance&rInstance, rttr::property&rProperty, std::string&rLabel, bool*bIsElementAndChanged)> Function)
{
	m_ResourcePropertyLookUp[FileExtension] = Function;
}

void PropertyRenderer::CreateArrayResourcePropertyLookUp(const std::string& FileExtension, std::function<bool(rttr::instance&rInstance, rttr::property&rProperty, rttr::variant_sequential_view&rArrView, unsigned int uiIndex, const std::string&rLabel)> Function)
{
	m_ArrayResourcePropertyLookUp[FileExtension] = Function;
}

std::string PropertyRenderer::GetResourceFileName(const std::string& FilePath) const
{
	std::string sFilePath = FilePath;
	if (sFilePath.empty())
		return "";

	// Return a file without the directories.
	size_t foundfilename = sFilePath.rfind('/');
	sFilePath = (foundfilename != std::string::npos) ? sFilePath.substr(foundfilename + 1) : sFilePath;

	// Removes the full file name without extension
	size_t founddot = sFilePath.find(".");
	sFilePath = (founddot != std::string::npos) ? sFilePath.substr(0, founddot) : sFilePath;
	return  sFilePath.substr(0, sFilePath.find_last_of('.'));
}

void PropertyRenderer::FirePropertyCommand(rttr::instance& targetInstance, rttr::property& targetProperty, rttr::variant& redoValue) const
{
	EditorWorld* pActiveEditorWorld = m_pEditor->GetEditorWorld();
	EditorPropertyCommand* pNewPropertyCommand = new EditorPropertyCommand(targetInstance, targetProperty, redoValue);
	pActiveEditorWorld->GetCommandManager().AddCommand(pNewPropertyCommand);
}
