#include "TestWorld.h"

#include "Core/Utils/Utils.h"
#include "Pickups/TriWeaponPickup.h"
#include "Pickups/Duplicator.h"
#include "Pickups/FreezePickup.h"
#include "Core/ECS/Components/AudioSource.h"
#include "Pickups/BombPickup.h"
#include "Pickups/BuildPickup.h"
#include "Core/ECS/Entities/Camera.h"
#include "Core/ECS/Components/BoxCollider.h"
#include "Core/ECS/Components/VoxRenderer.h"

#include "Humanoids/Enemies/Monster.h"
#include "Humanoids/Players/Player.h"

#include "General/Managers/GameManager.h"
// #include "AI/Spawner/SpawnerEntity.h"
#include "AI/Spawner/Spawner.h"

#include "Prefabs/BoxPrefab.h"
#include "Prefabs/KeyPrefab.h"

#include <Core/ECS/World.h>
#include <Core/Application.h>

TestWorld::TestWorld(Application* pApp) :
	World(pApp)
{ }

void TestWorld::Initialize()
{
	World::Initialize();

	SetGroundTexturePath(GetApplication()->GetPlatform().GetBasePath() + "Content/Textures/Grass.png");

	//auto orbitCamera = GetMainCamera()->AddComponent<CameraOrbit>();

	/* Test world init stuff */
	GetMainCamera()->GetTransform()->SetPosition(Vector3(0, -50, -100));
	GetMainCamera()->GetTransform()->SetEulerAngles(Vector3(45, 0, 0));

	const auto player = SpawnEntity<Player>(Vector3(25, 10, 25), Vector3(), Vector3(1, 1, 1));

	auto pGameManager = SpawnEntity<GameManager>(Vector3(0.f), Vector3(0.f), Vector3(1.f));

	pGameManager->SetPlayer(player, 0);
	//orbitCamera->SetTarget(player);

	SpawnEntity<BoxPrefab>(Vector3(50.0f, 1.0f, 80.0f), Vector3(0.f), Vector3(1.f));
	SpawnEntity<BoxPrefab>(Vector3(130.0f, 1.0f, 80.0f), Vector3(0.f), Vector3(1.f));
	SpawnEntity<BoxPrefab>(Vector3(50.0f, 1.0f, 120.0f), Vector3(0.f), Vector3(1.f));
	SpawnEntity<BoxPrefab>(Vector3(130.0f, 1.0f, 120.0f), Vector3(0.f), Vector3(1.f));

	// Keep in mind Rotation Y is in the X!
	auto key1 = SpawnEntity<KeyPrefab>(Vector3(50.0f, 12.0f, 80.0f), Vector3(0.0f, 270.0f, 0.0f), Vector3(1.f));
	auto key2 = SpawnEntity<KeyPrefab>(Vector3(130.0f, 12.0f, 80.0f), Vector3(0.0f, 270.0f, 0.0f), Vector3(1.f));
	auto key3 = SpawnEntity<KeyPrefab>(Vector3(50.0f, 12.0f, 120.0f), Vector3(0.0f, 270.0f, 0.0f), Vector3(1.f));
	auto key4 = SpawnEntity<KeyPrefab>(Vector3(130.0f, 12.0f, 120.0f), Vector3(0.0f, 270.0f, 0.0f), Vector3(1.f));

	/* Wave System settings - Spawner is now a component and the entity is basically a trigger box */
	Entity* pWaveNumberOne = SpawnEntity<Entity>(Vector3(120.0f, 1.0f, 200.0f), Vector3(), Vector3(1.f));
	pWaveNumberOne->SetName("WaveNumberOne");

	// first we have to create four enemies

	VoxModel* pStructureModel = GetApplication()->GetResourceManager().LoadVox("Content/Enviourment_Models/Monu1.vox");
	Entity* pStructure = SpawnEntity<Entity>(Vector3(80.0f, 0, 180.0f) + pStructureModel->GetFrame(0)->GetSize() * 0.5f, Vector3(0.f), Vector3(1.f));
	VoxRenderer* renderer = pStructure->AddComponent<VoxRenderer>();
	BoxCollider* pCollider = pStructure->AddComponent<BoxCollider>();
	pCollider->SetBoxSize(pStructureModel->GetFrame(0));
	renderer->SetModel(pStructureModel);
	pStructure->SetStatic(true);
	pCollider->SetLayer(CollisionLayer::CL_OBSTACLES);

	// Entity* pPoolmanager = SpawnEntity<Entity>(Vector3(0.0f), Vector3(0.0f), Vector3(1.f));
	// pPoolmanager->AddComponent<PoolManager>();
	// pPoolmanager->SetName("PoolManager");

	AudioSource* pSource = pWaveNumberOne->AddComponent<AudioSource>();

	pSource->SetFilePath("Content/Audio/Music/Battle_Normal.ogg");
	pSource->SetLooping(true);
	pSource->SetLoopCount(-1);
	pSource->SetLoopPoints(157571, 2671259);

	SpawnEntity<TriWeaponPickup>(Vector3(200.0f, 1.0f, 140.0f), Vector3(), Vector3(1.f));

	SpawnEntity<Duplicator>(Vector3(40.0f, 1.0f, 200.0f), Vector3(), Vector3(1.f));

	SpawnEntity<FreezePickup>(Vector3(140.0f, 1.0f, 100.0f), Vector3(), Vector3(1.f));

	// TODO: Bomb pickup must blow up and now turn into a power-up
	SpawnEntity<BombPickup>(Vector3(140.0f, 1.0f, 120.0f), Vector3(), Vector3(1.f));

	SpawnEntity<BuildPickup>(Vector3(40.0f, 1.0f, 120.0f), Vector3(), Vector3(1.f));
}
