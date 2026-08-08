# Bit Buster translation units. Kept explicit for the same reason as
# the engine list: a glob would pick up half-ported files.

# These four are on disk but were NOT in Game.vcxproj, so they never compiled:
#   AI/Spawner/SpawnerEntity.cpp, AI/Spawner/Wave.cpp,
#   AI/States/AIAttackState.cpp, AI/States/AIPatrolState.cpp
# SpawnerEntity calls OnDrawGizmos on a Component, which only Entity declares,
# so it cannot ever have built. Left out to match the original build exactly
# rather than repairing dead code.
set(VOXAGINE_GAME_SOURCES
    ${CMAKE_CURRENT_SOURCE_DIR}/Game/Source/AI/FiniteStateMachine.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/Game/Source/AI/Spawner/Spawner.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/Game/Source/AI/Spawner/SpawnerManager.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/Game/Source/Gameplay/Custimization/Base/BaseLoadoutBehavior.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/Game/Source/Gameplay/States/GM_LoadoutState.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/Game/Source/Gameplay/States/GM_PlayingState.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/Game/Source/Gameplay/Wall/BoundingWall.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/Game/Source/General/BGMTrigger.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/Game/Source/General/CameraMultiplayer.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/Game/Source/General/ExplosionTrigger.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/Game/Source/General/FlashBehavior.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/Game/Source/General/GameplayTimer.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/Game/Source/General/KillTrigger.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/Game/Source/General/Managers/GameManager.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/Game/Source/General/Managers/GameStateManager.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/Game/Source/General/Managers/WeaponManager.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/Game/Source/General/OpenLevelTrigger.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/Game/Source/Humanoids/Enemies/AutoMoveMonster.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/Game/Source/Humanoids/Enemies/HordeMonster.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/Game/Source/Humanoids/Enemies/LongNeckMonster.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/Game/Source/Humanoids/Enemies/Monster.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/Game/Source/Humanoids/Enemies/OgreMonster.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/Game/Source/Humanoids/Enemies/RandomMonster.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/Game/Source/Humanoids/Enemies/RangeMonster.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/Game/Source/Humanoids/Enemies/SpiderMonster.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/Game/Source/Humanoids/Enemies/UmbrellaMonster.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/Game/Source/Humanoids/Humanoid.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/Game/Source/Humanoids/ParticleCorpse.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/Game/Source/Humanoids/Players/Player.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/Game/Source/Humanoids/States/Hum_DashState.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/Game/Source/Humanoids/States/Hum_IdleState.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/Game/Source/Humanoids/States/Hum_MoveState.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/Game/Source/Humanoids/States/Hum_ThrowState.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/Game/Source/Humanoids/States/Mon_IdleState.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/Game/Source/Humanoids/States/Mon_MeleeAttackState.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/Game/Source/Humanoids/States/Mon_MoveState.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/Game/Source/Humanoids/States/Mon_RangeAttackState.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/Game/Source/main.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/Game/Source/Pickups/Ammo.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/Game/Source/Pickups/Bomb.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/Game/Source/Pickups/BombPickup.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/Game/Source/Pickups/BuildBlock.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/Game/Source/Pickups/BuildPickup.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/Game/Source/Pickups/Duplicator.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/Game/Source/Pickups/FreezePickup.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/Game/Source/Pickups/GenericPickup.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/Game/Source/Pickups/HealthPack.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/Game/Source/Pickups/TriWeaponPickup.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/Game/Source/Pickups/WeaponPickup.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/Game/Source/Prefabs/AimPrefab.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/Game/Source/Prefabs/BoxPrefab.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/Game/Source/Prefabs/DoorPrefab.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/Game/Source/Prefabs/KeyPrefab.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/Game/Source/Prefabs/PortalPrefab.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/Game/Source/Prefabs/RecallPrefab.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/Game/Source/Prefabs/StaticRendererPrefab.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/Game/Source/Prefabs/TestDummy.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/Game/Source/TestWorld.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/Game/Source/UI/CanvasSwitch.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/Game/Source/UI/ComboIcon.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/Game/Source/UI/ComboSliderUI.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/Game/Source/UI/ComboUI.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/Game/Source/UI/DashCooldownComponent.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/Game/Source/UI/HealthComponent.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/Game/Source/UI/HealthUI.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/Game/Source/UI/HighScoreShowUI.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/Game/Source/UI/HighScoreUI.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/Game/Source/UI/LevelSelect/LevelSelectCanvas.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/Game/Source/UI/Loadout.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/Game/Source/UI/MainMenu/MainMenuManagerComponent.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/Game/Source/UI/MainMenu/StartToJoinPlayerComponent.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/Game/Source/UI/OnBoarderUI.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/Game/Source/UI/PausedScreen/PauseScreenHandler.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/Game/Source/UI/Spinner.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/Game/Source/UI/SplashScreen/SplashScreenHandler.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/Game/Source/UI/States/MenuState.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/Game/Source/UI/WorldSwitch.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/Game/Source/VoxApp.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/Game/Source/Weapons/Bullet.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/Game/Source/Weapons/EnemyBullet.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/Game/Source/Weapons/Projectile.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/Game/Source/Weapons/Weapon.cpp
)
