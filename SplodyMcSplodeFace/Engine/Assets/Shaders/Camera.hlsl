float3 GetOrigin(matrix m4View, float2 v2ScreenPosition, float2 v2ScreenSize, float fProjectionValue, float fAspectRatio) {
	// Orthographic mode
	if (fProjectionValue == 0.0) {
		return mul(m4View, float4((v2ScreenPosition.xy - v2ScreenSize.xy * 0.5) * fAspectRatio * 0.1, -1000.0, 1.0)).xyz;
	}
	
	return mul(m4View, float4(0.0, 0.0, 0.0, 1.0)).xyz;
}

float3 GetRay(matrix m4View, float2 v2ScreenPosition, float2 v2ScreenSize, float fProjectionValue, float fAspectRatio) {
	// Orthographic mode
	if (fProjectionValue == 0.0) {
		return mul(m4View, float4(0.0, 0.0, 1.0, 0.0)).xyz;
	}
	
	return mul(m4View, float4(
		(2.0 * v2ScreenPosition.x / v2ScreenSize.x - 1.0) * fAspectRatio,
		(1.0 - 2.0 * v2ScreenPosition.y / v2ScreenSize.y),
		fProjectionValue,
		0
	)).xyz;
}