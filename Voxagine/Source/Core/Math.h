#pragma once
#include <emmintrin.h>

// GLM
#define GLM_FORCE_LEFT_HANDED
#define GLM_ENABLE_EXPERIMENTAL

#pragma warning(push, 0)        
#include <External/glm/glm.hpp>
#include <External/glm/gtx/transform.hpp>
#include <External/glm/gtx/norm.hpp>
#include <External/glm/gtc/random.hpp>
#pragma warning(pop)

typedef glm::vec4 Vector4;
typedef glm::vec3 Vector3;
typedef glm::vec2 Vector2;

typedef glm::ivec4 IVector4;
typedef glm::ivec3 IVector3;
typedef glm::ivec2 IVector2;

typedef glm::uvec4 UVector4;
typedef glm::uvec3 UVector3;
typedef glm::uvec2 UVector2;

typedef glm::quat Quaternion;
typedef glm::mat4 Matrix4;

struct VColor
{
	VColor() { inst.Color = 0; }
	VColor(uint32_t uiColor) { inst.Color = uiColor; }
	VColor(unsigned char r, unsigned char g, unsigned char b, unsigned char a)
	{
		inst.Colors.r = r;
		inst.Colors.g = g;
		inst.Colors.b = b;
		inst.Colors.a = a;
	}

	VColor(float r, float g, float b, float a)
	{
		inst.Colors.r = static_cast<char>(255.f * r);
		inst.Colors.g = static_cast<char>(255.f * g);
		inst.Colors.b = static_cast<char>(255.f * b);
		inst.Colors.a = static_cast<char>(255.f * a);
	}
	VColor(Vector4 color)
	{
		inst.Colors.r = static_cast<char>(255.f * color.r);
		inst.Colors.g = static_cast<char>(255.f * color.g);
		inst.Colors.b = static_cast<char>(255.f * color.b);
		inst.Colors.a = static_cast<char>(255.f * color.a);
	}

	Vector4 GetVector4() const
	{
		Vector4 ret;
		ret.r = static_cast<float>(inst.Colors.r) / 255.f;
		ret.g = static_cast<float>(inst.Colors.g) / 255.f;
		ret.b = static_cast<float>(inst.Colors.b) / 255.f;
		ret.a = static_cast<float>(inst.Colors.a) / 255.f;
		return ret;
	}


	union RGBAUnion
	{
		uint32_t Color;
		struct RBGAColor
		{
			unsigned char r, g, b, a;
		} Colors;
	} inst;
};

inline int ftoi_sse1(float f)
{
	return _mm_cvtt_ss2si(_mm_load_ss(&f));     // SSE1 instructions for float->int
}