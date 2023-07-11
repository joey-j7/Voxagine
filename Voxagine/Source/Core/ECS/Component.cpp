#include "pch.h"
#include "Core/ECS/Component.h"

#include <External/rttr/registration>
#include <External/rttr/policy.h>
#include "Core/MetaData/PropertyTypeMetaData.h"

RTTR_REGISTRATION
{
	rttr::registration::class_<Component>("Component")
			.constructor<Entity*>()(rttr::policy::ctor::as_raw_ptr)
			.property("Enabled", &Component::IsComponentOnlyEnabled, &Component::SetEnabled) ( RTTR_PUBLIC )
			.property("Component Tags", &Component::m_Tags) (RTTR_PUBLIC);
}

Component::Component(Entity* pOwner)
{
	m_pOwner = pOwner;
	m_bEnabled = true;
	m_bIsInitialized = false;
	m_pTransform = (pOwner) ? pOwner->GetTransform(): nullptr;
}

bool Component::IsEnabled(bool bWithParent) const
{
	if (bWithParent)
	{
		if (!m_pOwner->IsEnabled())
			return false;
	}
	return m_bEnabled;
}

void Component::SetEnabled(bool bEnabled)
{
	if (bEnabled && !m_bEnabled)
	{
		bool foundAllDeps = true;
		for (rttr::type compType : m_Dependencies)
		{
			if (!FindDependency(compType))
			{
				foundAllDeps = false;
				break;
			}
		}

		if (foundAllDeps)
		{
			m_bEnabled = bEnabled;
			OnEnabled();
		}
	}
	else if (!bEnabled && m_bEnabled)
	{
		m_bEnabled = bEnabled;
		OnDisabled();
	}
}

bool Component::HasTag(std::string tag)
{
	std::vector<std::string>::iterator iter = std::find(m_Tags.begin(), m_Tags.end(), tag);
	return iter != m_Tags.end();
}

void Component::DependencyCheck(Component* pComponent, bool bAdded)
{
	if (pComponent == this && bAdded)
	{
		for (rttr::type compType : m_Dependencies)
		{
			if (!FindDependency(compType))
			{
				SetEnabled(false);
				return;
			}
		}
	}
	else if (!bAdded)
	{
		for (rttr::type compType : m_Dependencies)
		{
			if (pComponent->get_type() == compType)
			{
				SetEnabled(false);
				return;
			}
		}
	}
}

bool Component::FindDependency(rttr::type depType)
{
	for (Component* pEntityComponent : GetOwner()->GetComponents())
	{
		if (pEntityComponent->get_type() == this->get_type()) continue;
		if (depType == pEntityComponent->get_type())
		{
			return true;
		}
	}
	return false;
}
