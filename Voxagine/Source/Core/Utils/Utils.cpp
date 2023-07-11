#include "pch.h"
#include "Utils.h"

Quaternion Utils::FindLookAtRotation(Vector3 vStart, Vector3 vTarget)
{
	Quaternion qTargetRotation = Quaternion();
	Vector3 lookDirection = vTarget - vStart;
	if (glm::length(lookDirection) != 0)
	{
		lookDirection = glm::normalize(lookDirection);
		const float fAngle = atan2(lookDirection.x, lookDirection.z);

		qTargetRotation = glm::angleAxis(fAngle, Vector3(0.f, 1.f, 0.f));
		//qTargetRotation = Quaternion::CreateFromAxisAngle(Vector3(0.f, 1.f, 0.f), fAngle);
		//CLEANUP

		return qTargetRotation;
	}

	return qTargetRotation;
}

Vector3 Utils::MoveTowards(Vector3 current, Vector3 target, float maxDistanceDelta)
{
	const Vector3 a = target - current;
	const float magnitude = glm::length(a);
	if (magnitude <= maxDistanceDelta || magnitude == 0.0f)
	{
		return target;
	}
	return current + a / magnitude * maxDistanceDelta;
}


bool Utils::CheckDerivedType(const rttr::type variantType, const rttr::type& rTypeCheck)
{
	// if array return
	if (rTypeCheck.is_sequential_container())
		return CheckArrayDerivedType(variantType, rTypeCheck);
	if (rTypeCheck.is_associative_container())
		// TODO support maps
		return false;

	const auto bIsPointer = variantType.is_pointer();
	const auto TypeCheck = rTypeCheck.is_pointer() ? rTypeCheck.get_raw_type() : rTypeCheck;

	// if we already have a base class
	if ((bIsPointer ? variantType.get_raw_type() : variantType) == TypeCheck)
		return true;

	return (rTypeCheck.is_pointer() ? rTypeCheck.get_raw_type() : rTypeCheck).is_base_of(variantType.is_pointer() ? variantType.get_raw_type() : variantType);
}

bool Utils::CheckArrayDerivedType(rttr::property& rProperty, const rttr::type& rTypeCheck)
{
	if (rProperty.get_type().is_sequential_container())
	{
		return CheckArrayDerivedType(rProperty.get_type(), rTypeCheck);
	}

	return false;
}

Vector3 Utils::SphericalRand(float fRadius, float fThetaMin, float fThetaMax, float fPhiMin, float fPhiMax)
{
	float theta = glm::linearRand(fThetaMin, fThetaMax);
	float phi = std::acos(glm::linearRand(fPhiMin, fPhiMax));

	float x = std::sin(phi) * std::cos(theta);
	float y = std::sin(phi) * std::sin(theta);
	float z = std::cos(phi);

	return Vector3(x, y, z) * fRadius;
}

bool Utils::CheckArrayDerivedType(rttr::type rType, const rttr::type& rTypeCheck)
{
	auto templateArgs = rType.get_template_arguments();
	for (const auto& templateArgType : templateArgs)
	{
		if (CheckDerivedType(templateArgType, rTypeCheck)) return true;
	}

	return false;
}

#ifdef EDITOR
#include "External/imgui/imgui.h"
#include "External/imgui/imgui_internal.h"
void Utils::Tooltip(const rttr::property& prop)
{
	auto& g = *ImGui::GetCurrentContext();
	if (!g.DragDropActive && ImGui::IsItemHovered())
	{
		// ImGui::SetMouseCursor(ImGuiMouseCursor_Help);
		const auto tooltip = prop.get_metadata("Tooltip");
		if (tooltip)
		{
			ImGui::BeginTooltip();
			ImGui::TextUnformatted(tooltip.to_string().c_str());
			ImGui::EndTooltip();
		}
	}
}

#endif
