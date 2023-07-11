#pragma once

#include <External/imguizmo/ImGuizmo.h>

class SnappingTool
{
private:
	struct SnappingValues { float SnapValue[3]; };

public:
	SnappingTool();
	~SnappingTool();

	void SetUsingIndex(ImGuizmo::OPERATION TargetOperation, size_t UsingIndex);
	size_t GetUsingIndex(ImGuizmo::OPERATION TargetOperation);

	void SetSnappingValue(ImGuizmo::OPERATION TargetOperation, size_t ValueIndex, float NewValue);
	float GetSnappingValue(ImGuizmo::OPERATION TargetOperation, size_t ValueIndex);
	float* GetSnappingValuePtr(ImGuizmo::OPERATION TargetOperation, size_t ValueIndex);

	void SetActiveSnappingValue(ImGuizmo::OPERATION TargetOperation, float NewValue);
	float GetActiveSnappingValue(ImGuizmo::OPERATION TargetOperation);
	float* GetActiveSnappingValuePtr(ImGuizmo::OPERATION TargetOperation);

private:
	SnappingValues* GetActiveSnappingValues(ImGuizmo::OPERATION TargetOperation);
	SnappingValues* GetSnappingValues(ImGuizmo::OPERATION TargetOperation, size_t UsingIndex);
private:
	size_t m_ActiveSnappingValues[3];

	SnappingValues TranslateValues[3];
	SnappingValues RotateValues;
	SnappingValues ScalarValues;
};