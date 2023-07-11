#include "pch.h"
#include "VClass.h"

// #include "MetaData/PropertyTypeMetaData.h"

#include <External/rttr/registration>

RTTR_REGISTRATION
{
	rttr::registration::class_<VClass>("VClass")
		.constructor<>()(rttr::policy::ctor::as_object)
		.property("EntityRef", &VClass::m_pEntity)
		.property("ShouldInstantiate", &VClass::m_bInstantiate)
		.property("InstantiateType", &VClass::GetInstantiateType, &VClass::SetInstantiateType, rttr::registration::public_access)
		.method("Instantiate", &VClass::InstantiateType)
		.method("SetInstantiateType", &VClass::set_derived_type)
		.method("IsSubClass", &VClass::is_sub_class)
		.method("IsSame", &VClass::is_same)
		.method("GetInnerType", &VClass::get_inner_type)
		.method("GetDerivedTypes", &VClass::get_derived_types);
}

VClass::VClass() { }

void VClass::SetInstantiateType(std::string sNewType)
{	
	rttr::type CurrentType = m_rInstantiateType;
	const rttr::type NewType = rttr::type::get_by_name(sNewType);
	if(CurrentType.is_valid() && CurrentType.is_base_of(NewType))
	{
		m_rInstantiateType = NewType;
		m_InstantiateType = sNewType;
		// return;
	}

	// TODO use the logger to say that the type is not of base type of this class.
}

void VClass::set_derived_type(rttr::type sNewType)
{
	rttr::type CurrentType = rttr::type::get<VClass>();
	if (CurrentType.is_valid() && CurrentType.is_base_of(sNewType) && sNewType.is_valid())
	{
		m_rInstantiateType = sNewType;
		m_InstantiateType = m_rInstantiateType.get_name().to_string();
		// return;
	}

	// TODO use the logger to say that the type is not of base type of this class.
}

std::string VClass::GetInstantiateType() const
{
	return m_InstantiateType;
}

bool VClass::is_sub_class() const
{
	return m_bIsSubClass;
}
