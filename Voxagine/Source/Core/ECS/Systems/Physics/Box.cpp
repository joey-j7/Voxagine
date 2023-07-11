#include "pch.h"
#include "Box.h"

Box::Box(BoxCollider* pCollider)
{
	Min = pCollider->GetBoxMin();
	Max = pCollider->GetBoxMax();
}

bool Box::Intersects(const Box& boxB, Manifold& manifold)
{
	manifold.Normal = Vector3(0, 0, 0);
	manifold.Overlap = 0.f;

	if (Intersects(boxB))
	{
		Vector3 size = GetSize();
		Vector3 boxBSize = boxB.GetSize();
		float overlapX, overlapY, overlapZ;

		if (Min.x < boxB.Min.x)
			overlapX = boxB.Min.x - (Min.x + size.x);
		else overlapX = boxB.Min.x + boxBSize.x - Min.x;

		if (Min.y < boxB.Min.y)
			overlapY = boxB.Min.y - (Min.y + size.y);
		else overlapY = boxB.Min.y + boxBSize.y - Min.y;

		if (Min.z < boxB.Min.z)
			overlapZ = boxB.Min.z - (Min.z + size.z);
		else overlapZ = boxB.Min.z + boxBSize.z - Min.z;

		Vector3 normal = Vector3(0.f);
		if (fabs(overlapX) <= fabs(overlapY) && fabs(overlapX) <= fabs(overlapZ))
		{
			manifold.Overlap = fabs(overlapX);
			manifold.Normal.x = overlapX;
		}
		else if (fabs(overlapY) <= fabs(overlapZ))
		{
			manifold.Overlap = fabs(overlapY);
			manifold.Normal.y = overlapY;
		}
		else
		{
			manifold.Overlap = fabs(overlapZ);
			manifold.Normal.z = overlapZ;
		}
		
		manifold.Normal = glm::normalize(manifold.Normal);
		return true;
	}
	return false;
}

inline bool Box::Intersects(const Box& boxB)
{
	return (Max.x > boxB.Min.x && Min.x < boxB.Max.x &&
		Max.y > boxB.Min.y && Min.y < boxB.Max.y &&
		Max.z > boxB.Min.z && Min.z < boxB.Max.z);
}