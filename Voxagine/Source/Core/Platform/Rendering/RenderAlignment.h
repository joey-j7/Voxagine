#pragma once

enum RenderAlignment
{
	RA_CENTERED,
	RA_TOPLEFT,
	RA_TOPCENTER,
	RA_TOPRIGHT,
	RA_RIGHTCENTER,
	RA_LEFTCENTER,
	RA_BOTTOMLEFT,
	RA_BOTTOMCENTER,
	RA_BOTTOMRIGHT
};

static Vector2 GetNormRenderAlignment(RenderAlignment alignment)
{
	switch (alignment)
	{
	case RA_TOPLEFT:
		return Vector2(0.f, 0.f);
		break;
	case RA_TOPCENTER:
		return Vector2(0.5f, 0.f);
		break;
	case RA_TOPRIGHT:
		return Vector2(1.f, 0.f);
		break;
	case RA_RIGHTCENTER:
		return Vector2(1.f, 0.5f);
		break;
	case RA_LEFTCENTER:
		return Vector2(0.f, 0.5f);
		break;
	case RA_BOTTOMLEFT:
		return Vector2(0.f, 1.f);
		break;
	case RA_BOTTOMCENTER:
		return Vector2(0.5f, 1.f);
		break;
	case RA_BOTTOMRIGHT:
		return Vector2(1.f, 1.f);
		break;
	default:
		return Vector2(0.5f, 0.5f);
		break;
	}
}