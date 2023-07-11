#include "pch.h"
#include "EditorPropertyCommand.h"
#include <utility>

#include "Core/ECS/Entity.h"

EditorPropertyCommand::EditorPropertyCommand(rttr::instance targetInstance, rttr::property targetProperty, rttr::variant redoValue)
	: m_TargetInstance(targetInstance)
	, m_TargetType(targetInstance.get_derived_type())
	, m_Property(targetProperty)
	, m_RedoValue(std::move(redoValue))
{
	Entity* pTargetEntity = targetInstance.try_convert<Entity>();
	if (pTargetEntity != nullptr)
	{
		m_TargetEntityID = pTargetEntity->GetId();
		m_pWorld = pTargetEntity->GetWorld();
	}
	else if (const auto pTargetComponent = targetInstance.try_convert<Component>())
	{
		m_TargetEntityID = pTargetComponent->GetOwner()->GetId();
		m_pWorld = pTargetComponent->GetOwner()->GetWorld();
	} else if (const auto pTargetClass = targetInstance.try_convert<VClass>())
	{
		const int64_t ui64 = pTargetClass && pTargetClass->GetOwner() ? pTargetClass->GetOwner()->GetId() : -1;
		assert(ui64 != -1 && "There is no reference to an Entity! Did you forget to call VClass::SetOwner()?");

		m_TargetEntityID =  static_cast<uint64_t>(ui64);
		m_pWorld = pTargetClass->GetOwner()->GetWorld();
	}

	m_UndoValue = m_Property.get_value(targetInstance);
}

EditorPropertyCommand::~EditorPropertyCommand()
{
}

void EditorPropertyCommand::Redo()
{
	SetValue(m_RedoValue);
}

void EditorPropertyCommand::Undo()
{
	SetValue(m_UndoValue);
}

void EditorPropertyCommand::SetValue(rttr::variant & valueToSet)
{
	Entity* pTargetEntity = (m_pWorld) ? m_pWorld->FindEntity(m_TargetEntityID) : nullptr;
	bool IsEntityProperty = m_TargetType.is_derived_from<Entity>();

	if (IsEntityProperty)
	{
		rttr::instance PropertyHolderInstance = pTargetEntity;
		m_Property.set_value(PropertyHolderInstance, valueToSet);
	}
	else if (m_TargetType.is_derived_from<Component>())
	{
		rttr::instance PropertyHolderInstance = pTargetEntity->GetComponent(m_TargetType);
		m_Property.set_value(PropertyHolderInstance, valueToSet);
	}
	else
	{
		if(m_TargetInstance.is_valid())
			VX_UNUSED(m_Property.set_value(m_TargetInstance, valueToSet));
	}
}
