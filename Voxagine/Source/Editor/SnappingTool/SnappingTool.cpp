#include "pch.h"
#include "Editor/SnappingTool/SnappingTool.h"

SnappingTool::SnappingTool()
{
	m_ActiveSnappingValues[0] = 0;
	m_ActiveSnappingValues[1] = 0;
	m_ActiveSnappingValues[2] = 0;
}

SnappingTool::~SnappingTool()
{

}

void SnappingTool::SetUsingIndex(ImGuizmo::OPERATION TargetOperation, size_t UsingIndex)
{
	if (TargetOperation == ImGuizmo::BOUNDS)
		return;

	if (UsingIndex >= 0 && UsingIndex <= 2)
	{
		m_ActiveSnappingValues[TargetOperation] = UsingIndex;
	}
}

size_t SnappingTool::GetUsingIndex(ImGuizmo::OPERATION TargetOperation)
{
	if (TargetOperation == ImGuizmo::BOUNDS)
		return 0;

	return m_ActiveSnappingValues[TargetOperation];
}

void SnappingTool::SetSnappingValue(ImGuizmo::OPERATION TargetOperation, size_t ValueIndex, float NewValue)
{
	SnappingValues* snapvalues = GetSnappingValues(TargetOperation, ValueIndex);
	if (snapvalues == nullptr)
		return;

	if (TargetOperation == ImGuizmo::TRANSLATE)
	{
		for (size_t valueIt = 0; valueIt != 3; ++valueIt)
		{
			snapvalues->SnapValue[valueIt] = NewValue;
		}
	}
	else
	{
		snapvalues->SnapValue[ValueIndex] = NewValue;
	}
}

float SnappingTool::GetSnappingValue(ImGuizmo::OPERATION TargetOperation, size_t ValueIndex)
{
	float* snapvalues = GetSnappingValuePtr(TargetOperation, ValueIndex);
	if (snapvalues == nullptr)
		return 0.0f;

	return *snapvalues;
}

float * SnappingTool::GetSnappingValuePtr(ImGuizmo::OPERATION TargetOperation, size_t ValueIndex)
{
	SnappingValues* snapvalues = GetSnappingValues(TargetOperation, ValueIndex);
	if (snapvalues == nullptr)
		return nullptr;
	
	return (TargetOperation == ImGuizmo::TRANSLATE) ? &snapvalues->SnapValue[0] : &snapvalues->SnapValue[ValueIndex];
}

void SnappingTool::SetActiveSnappingValue(ImGuizmo::OPERATION TargetOperation, float NewValue)
{
	SetSnappingValue(TargetOperation, GetUsingIndex(TargetOperation), NewValue);
}

float SnappingTool::GetActiveSnappingValue(ImGuizmo::OPERATION TargetOperation)
{
	return GetSnappingValue(TargetOperation, GetUsingIndex(TargetOperation));
}

float * SnappingTool::GetActiveSnappingValuePtr(ImGuizmo::OPERATION TargetOperation)
{
	return GetSnappingValuePtr(TargetOperation, GetUsingIndex(TargetOperation));
}

SnappingTool::SnappingValues* SnappingTool::GetActiveSnappingValues(ImGuizmo::OPERATION TargetOperation)
{
	return GetSnappingValues(TargetOperation, GetUsingIndex(TargetOperation));
}

SnappingTool::SnappingValues* SnappingTool::GetSnappingValues(ImGuizmo::OPERATION TargetOperation, size_t UsingIndex)
{
	if (TargetOperation == ImGuizmo::TRANSLATE)
	{
		if (UsingIndex >= 0 && UsingIndex <= 2)
			return &TranslateValues[UsingIndex];
	}

	if (TargetOperation == ImGuizmo::ROTATE)
		return &RotateValues;

	if (TargetOperation == ImGuizmo::SCALE)
		return &ScalarValues;

	return nullptr;
}

