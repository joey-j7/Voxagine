#include "gtest/gtest.h"

#include "Core/ECS/Systems/Physics/PhysicsSystem.h"
#include "Core/ECS/Systems/Physics/Box.h"
#include "Core/ECS/Systems/Physics/Sphere.h"
#include "Core/ECS/Systems/Physics/HitResult.h"

#include <chrono>

// Physics world scaffolding
class PhysicsSystemTest : public ::testing::Test
{
protected:
	PhysicsSystem* m_pPhysicsSystem;
	std::vector<BoxCollider*> Collider;

protected:
	PhysicsSystemTest() :
		m_pPhysicsSystem(nullptr)
	{}

	void SetUp() override
	{
		m_pPhysicsSystem = new PhysicsSystem(nullptr, Vector3(1, 1, 1), 1, 1);
		for (int x = 0; x < 3; ++x)
		{
			for (int y = 0; y < 3; ++y)
			{
				for (int z = 0; z < 3; ++z)
				{
					Entity* pEntity = new Entity(nullptr);
					BoxCollider* pCollider = new BoxCollider(pEntity);
					pCollider->SetBoxSize(Vector3((x + 1) * 10, (x + 1) * 10, (x + 1) * 10));
					Vector3 arrMultiply(x, y, z);
					pCollider->GetTransform()->SetPosition(pCollider->GetBoxSize() * arrMultiply);
					m_pPhysicsSystem->AddComponent(pCollider);
					Collider.push_back(pCollider);
				}
			}
		}
	}

	void TearDown() override
	{
		for (BoxCollider* pColl : Collider)
		{
			pColl->Destroyed(pColl);
			delete pColl->GetOwner();
		}

		delete m_pPhysicsSystem;
	}
};

TEST_F(PhysicsSystemTest, RayCast)
{
	//Negative ray test
	Vector3 start(-10, 0, 0);
	Vector3 dir(1, 0, 0);
	HitResult result;
	bool ret = m_pPhysicsSystem->RayCast(start, dir, result, 1000.f);
	EXPECT_EQ(ret, true);
	EXPECT_NEAR(result.HitPoint.x, -5.f, 0.0000005);
	EXPECT_NEAR(result.HitPoint.y, 0.f, 0.0000005);
	EXPECT_NEAR(result.HitPoint.z, 0.f, 0.0000005);

	//Positive ray test
	start = Vector3(7, 0, 0);
	dir = Vector3(-1, 0, 0);
	result.HitEntity = nullptr;
	result.HitPoint = Vector3(0, 0, 0);
	ret = m_pPhysicsSystem->RayCast(start, dir, result, 1000.f);
	EXPECT_EQ(ret, true);
	EXPECT_NEAR(result.HitPoint.x, 5.f, 0.0000005);
	EXPECT_NEAR(result.HitPoint.y, 0.f, 0.0000005);
	EXPECT_NEAR(result.HitPoint.z, 0.f, 0.0000005);

	//Ray in object test
	start = Vector3(4, 0, 0);
	dir = Vector3(1, 0, 0);
	result.HitEntity = nullptr;
	result.HitPoint = Vector3(0, 0, 0);
	ret = m_pPhysicsSystem->RayCast(start, dir, result, 1000.f);
	EXPECT_EQ(ret, true);
	EXPECT_NEAR(result.HitPoint.x, 5.f, 0.0000005);
	EXPECT_NEAR(result.HitPoint.y, 0.f, 0.0000005);
	EXPECT_NEAR(result.HitPoint.z, 0.f, 0.0000005);

	//Up ray test
	start = Vector3(0, -10, 0);
	dir = Vector3(0, 1, 0);
	result.HitEntity = nullptr;
	result.HitPoint = Vector3(0, 0, 0);
	ret = m_pPhysicsSystem->RayCast(start, dir, result, 1000.f);
	EXPECT_EQ(ret, true);
	EXPECT_NEAR(result.HitPoint.x, 0.f, 0.0000005);
	EXPECT_NEAR(result.HitPoint.y, -5.f, 0.0000005);
	EXPECT_NEAR(result.HitPoint.z, 0.f, 0.0000005);

	//Failed ray to negative test
	start = Vector3(-10, 0, 0);
	dir = Vector3(0, 0, 1);
	result.HitEntity = nullptr;
	result.HitPoint = Vector3(0, 0, 0);
	ret = m_pPhysicsSystem->RayCast(start, dir, result, 1000.f);
	EXPECT_EQ(ret, false);
	EXPECT_NEAR(result.HitPoint.x, 0.f, 0.0000005);
	EXPECT_NEAR(result.HitPoint.y, 0.f, 0.0000005);
	EXPECT_NEAR(result.HitPoint.z, 0.f, 0.0000005);

	//Failed ray length test
	start = Vector3(-10, 0, 0);
	dir = Vector3(1, 0, 0);
	result.HitEntity = nullptr;
	result.HitPoint = Vector3(0, 0, 0);
	ret = m_pPhysicsSystem->RayCast(start, dir, result, 2.f);
	EXPECT_EQ(ret, false);
	EXPECT_NEAR(result.HitPoint.x, 0.f, 0.0000005);
	EXPECT_NEAR(result.HitPoint.y, 0.f, 0.0000005);
	EXPECT_NEAR(result.HitPoint.z, 0.f, 0.0000005);
}

TEST_F(PhysicsSystemTest, OverlapBox)
{
	//OverlapBox capture all test
	std::vector<BoxCollider*> colliders;
	bool ret = m_pPhysicsSystem->OverlapBox(colliders, Vector3(0, 0, 0), Vector3(100, 100, 100));
	EXPECT_EQ(ret, true);
	EXPECT_EQ(colliders.size(), 27);

	//OverlapBox in object capture test
	colliders.clear();
	ret = m_pPhysicsSystem->OverlapBox(colliders, Vector3(0, 0, 0), Vector3(2, 2, 2));
	EXPECT_EQ(ret, true);
	EXPECT_EQ(colliders.size(), 1);

	//OverlapBox no capture test
	colliders.clear();
	ret = m_pPhysicsSystem->OverlapBox(colliders, Vector3(-10, -10, -10), Vector3(2, 2, 2));
	EXPECT_EQ(ret, false);
	EXPECT_EQ(colliders.size(), 0);

	//OverlapBox line box test
	colliders.clear();
	ret = m_pPhysicsSystem->OverlapBox(colliders, Vector3(-10, 0, 0), Vector3(10, 0, 0));
	EXPECT_EQ(ret, true);
	EXPECT_EQ(colliders.size(), 1);

	//OverlapBox rectangle test
	colliders.clear();
	ret = m_pPhysicsSystem->OverlapBox(colliders, Vector3(0, -10, 0), Vector3(5, 10, 5));
	EXPECT_EQ(ret, true);
	EXPECT_EQ(colliders.size(), 1);
}

TEST_F(PhysicsSystemTest, OverlapSphere)
{
	//OverlapSphere capture all test
	std::vector<BoxCollider*> colliders;
	bool ret = m_pPhysicsSystem->OverlapSphere(colliders, Vector3(0, 0, 0), 100);
	EXPECT_EQ(ret, true);
	EXPECT_EQ(colliders.size(), 27);

	//OverlapSphere in object capture test
	colliders.clear();
	ret = m_pPhysicsSystem->OverlapSphere(colliders, Vector3(0, 0, 0), 2);
	EXPECT_EQ(ret, true);
	EXPECT_EQ(colliders.size(), 1);

	//OverlapSphere no capture test
	colliders.clear();
	ret = m_pPhysicsSystem->OverlapSphere(colliders, Vector3(-10, -10, -10), 2);
	EXPECT_EQ(ret, false);
	EXPECT_EQ(colliders.size(), 0);

	//OverlapSphere collider corner test
	colliders.clear();
	ret = m_pPhysicsSystem->OverlapSphere(colliders, Vector3(-10, -10, -10), 9.f);
	EXPECT_EQ(ret, true);
	EXPECT_EQ(colliders.size(), 1);
}

TEST(PhysicsTest, BoxToBox)
{
	Box boxA, boxB;
	
	//Test intersection with 2 boxes inside each other
	boxA.Max = Vector3(10, 10, 10);
	boxA.Min = Vector3(0, 0, 0);

	boxB.Max = Vector3(10, 10, 10);
	boxB.Min = Vector3(0, 0, 0);

	bool ret = boxA.Intersects(boxB);
	EXPECT_EQ(ret, true);

	//Test intersection with collision diagonal
	boxA.Max = Vector3(15, 15, 15);
	boxA.Min = Vector3(9, 9, 9);

	boxB.Max = Vector3(10, 10, 10);
	boxB.Min = Vector3(0, 0, 0);

	ret = boxA.Intersects(boxB);
	EXPECT_EQ(ret, true);

	//Test intersection outside bounds
	boxA.Max = Vector3(15, 15, 15);
	boxA.Min = Vector3(10, 10, 10);

	boxB.Max = Vector3(10, 10, 10);
	boxB.Min = Vector3(0, 0, 0);

	ret = boxA.Intersects(boxB);
	EXPECT_EQ(ret, false);

	//Test intersection from top
	boxA.Max = Vector3(15, 15, 15);
	boxA.Min = Vector3(0, 5, 0);

	boxB.Max = Vector3(10, 10, 10);
	boxB.Min = Vector3(0, 0, 0);

	ret = boxA.Intersects(boxB);
	EXPECT_EQ(ret, true);
}

TEST(PhysicsTest, BoxToBoxManifold)
{
	Box boxA, boxB;
	Manifold manifold;

	//Test manifold with 2 boxes inside each other
	boxA.Max = Vector3(10, 10, 10);
	boxA.Min = Vector3(0, 0, 0);

	boxB.Max = Vector3(10, 10, 10);
	boxB.Min = Vector3(0, 0, 0);

	bool ret = boxA.Intersects(boxB, manifold);
	EXPECT_EQ(manifold.Normal, Vector3(1, 0, 0));
	EXPECT_EQ(manifold.Overlap, 10.f);

	//Test manifold with diagonal intersection
	boxA.Max = Vector3(15, 15, 15);
	boxA.Min = Vector3(9, 9, 9);

	boxB.Max = Vector3(10, 10, 10);
	boxB.Min = Vector3(0, 0, 0);

	ret = boxA.Intersects(boxB, manifold);
	EXPECT_EQ(manifold.Normal, Vector3(1, 0, 0));
	EXPECT_EQ(manifold.Overlap, 1.f);

	//Test manifold with failed intersection
	boxA.Max = Vector3(15, 15, 15);
	boxA.Min = Vector3(10, 10, 10);

	boxB.Max = Vector3(10, 10, 10);
	boxB.Min = Vector3(0, 0, 0);

	ret = boxA.Intersects(boxB, manifold);
	EXPECT_EQ(manifold.Normal, Vector3(0, 0, 0));
	EXPECT_EQ(manifold.Overlap, 0.f);

	//Test manifold with top collision
	boxA.Max = Vector3(15, 15, 15);
	boxA.Min = Vector3(0, 5, 0);

	boxB.Max = Vector3(10, 10, 10);
	boxB.Min = Vector3(0, 0, 0);

	ret = boxA.Intersects(boxB, manifold);
	EXPECT_EQ(manifold.Normal, Vector3(0, 1, 0));
	EXPECT_EQ(manifold.Overlap, 5.f);

	//Test manifold with negative box coordinates
	boxA.Max = Vector3(0, -5, 0);
	boxA.Min = Vector3(-15, -15, -15);

	boxB.Max = Vector3(0, 0, 0);
	boxB.Min = Vector3(-10, -10, -10);

	ret = boxA.Intersects(boxB, manifold);
	EXPECT_EQ(manifold.Normal, Vector3(0, -1, 0));
	EXPECT_EQ(manifold.Overlap, 5.f);
}

TEST(PhysicsTest, SphereToSphere)
{
	Sphere sphereA;
	Sphere sphereB;

	//Positive sphere overlap test
	sphereA.Center = Vector3(10, 0, 0);
	sphereA.fRadius = 5;

	sphereB.Center = Vector3(3, 0, 0);
	sphereB.fRadius = 3;

	bool ret = sphereA.Intersects(sphereB);
	EXPECT_EQ(ret, true);

	//Negative no intersection test
	sphereA.Center = Vector3(-10, -5, 0);
	sphereA.fRadius = 2;

	sphereB.Center = Vector3(0, 0, 0);
	sphereB.fRadius = 5;

	ret = sphereA.Intersects(sphereB);
	EXPECT_EQ(ret, false);

	//Perfect sphere overlap test
	sphereA.Center = Vector3(0, 0, 0);
	sphereA.fRadius = 1;

	sphereB.Center = Vector3(0, 0, 0);
	sphereB.fRadius = 1;

	ret = sphereA.Intersects(sphereB);
	EXPECT_EQ(ret, true);
}

TEST(PhysicsTest, SphereToBox)
{
	Sphere sphereA;
	Box boxB;

	//Sphere to box overlap test with line box
	sphereA.Center = Vector3(0, 0, 0);
	sphereA.fRadius = 1;

	boxB.Max = Vector3(2, 0, 0);
	boxB.Min = Vector3(0, 0, 0);

	bool ret = sphereA.Intersects(boxB);
	EXPECT_EQ(ret, true);

	//Sphere to box overlap negative coordinates test
	sphereA.Center = Vector3(0, -10, 0);
	sphereA.fRadius = 5;

	boxB.Max = Vector3(0, 0, 0);
	boxB.Min = Vector3(-6, -6, -6);

	ret = sphereA.Intersects(boxB);
	EXPECT_EQ(ret, true);

	//Sphere to box overlap fail test
	sphereA.Center = Vector3(0, 0, 0);
	sphereA.fRadius = 10;

	boxB.Max = Vector3(0, 10, 0);
	boxB.Min = Vector3(20, 20, 20);

	ret = sphereA.Intersects(boxB);
	EXPECT_EQ(ret, false);

	//Sphere to box corner test
	sphereA.Center = Vector3(0, 0, 0);
	sphereA.fRadius = 9;

	boxB.Max = Vector3(15, 15, 15);
	boxB.Min = Vector3(5, 5, 5);

	ret = sphereA.Intersects(boxB);
	EXPECT_EQ(ret, true);
}