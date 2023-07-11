#pragma once

#include "Core/ECS/Entity.h"
#include "Core/ECS/World.h"

#include "Core/Resources/Formats/VoxModel.h"

#include "Core/Objects/VClass.h"

//#include "Weapons/Bullet.h"

typedef int IMonsterType;

class Weapon;
class Bullet;

class WeaponManager : public Entity
{
public:
	friend class Bullet;
	friend class Weapon;
	friend class GM_LoadOutState;

	// Since we cannot use multiple instantiated objects of type Weapon on the player (only 1 component of the same type can be assigned)
	// We will need child classes of each weapon. Structs seemed faster and cleaner to use so we used them instead.
	// This is the base/parent class for all of the weapon types
	struct Type : VClass
	{
		enum MovementType
		{
			MT_STRAIGHT,
			MT_SINE,
		};

		enum ActivatedType
		{
			AT_NONE,
			AT_TEST,
		};

		// enum PassiveType
		// {
			
		// };

		// enum ActiveType
		// {
			
		// };

		bool IsUnlocked() const { return m_bUnlocked; }
		void SetUnlocked(bool bUnlocked);

		float GetCatchRange() const { return m_fCatchRange; };
		void SetCatchRange(float fCatchRange);

		float GetAutoCatchRange() const { return m_fAutoCatchRange; };
		void SetAutoCatchRange(float fCatchRange);

		float GetPerfectCatchRange() const { return m_fPerfectCatchRange; };
		void SetPerfectCatchRange(float fPerfectCatchRange);

		float GetMinPerfCatchSpeed() const { return m_fMinPerfCatchSpeed; };
		void SetMinPerfCatchSpeed(float fMinPerfCatchSpeed);

		void Initialize();

		float m_fCameraShake = 0.1f;
		float m_fMaxCameraShake = 0.6f;

		// Here we set the variables and data types the struct will have
		uint32_t m_uiMaxAmmo = 0;

		float m_fDamage = 0.f;
		float m_fShootingDelay = 0.f;

		float m_fBulletExplosionRange = 18.f;

		bool m_bIsCurrentWeapon = false;
		MovementType m_MovementType = MT_STRAIGHT;
		// RelicType m_RelicType = RT_NONE;
		// std::vector<PassiveTypes> m_PassiveTypes = {};
		// NOTE stacking has to be done elsewhere
		ActivatedType m_ActivatedType;

		float m_fTravelTime = 1.f;
		float m_fDecayTime = 1.4f;
		
		float m_fRotationSpeed = 6.f;

		float m_fPathAnimationTime = 1.f;
		float m_fPathAnimationAmplitude = 1.f;
		uint32_t m_uiPathAnimationLoopMax = 1;

		std::string m_combo0ModelPath = "Content/Models/Projectiles/Weapon_Blue_Version_1.vox";
		std::string m_combo1ModelPath = "Content/Models/Projectiles/Weapon_Blue_Version_2.vox";
		std::string m_combo2ModelPath = "Content/Models/Projectiles/Weapon_Blue_Version_3.vox";
		std::string m_combo3ModelPath = "Content/Models/Projectiles/Weapon_Blue_Version_4.vox";
		
		std::string m_ModelPath = "";
		std::string m_IconPath = "";

		bool m_bSelected = false;
		bool m_bUnlocked = false;
		bool m_bActivated = false;
		float m_fResourceAmount = 1.0f;

		float m_fCatchRange = 20.f;
		float m_fAutoCatchRange = 20.0f;
		float m_fPerfectCatchRange = 20.f;

		float m_fMinPerfCatchSpeed = 10.f;

		// RTTR for VClass parent must be enabled so that the dropdown option for the struct appears
		RTTR_ENABLE(VClass);
		RTTR_REGISTRATION_FRIEND;
	};

	WeaponManager(World* pWorld);

	void Awake() override;
	void Start() override;
	
	const Type* GetCurrentType() const;

	size_t GetTypeID() const;
	void SetTypeID(size_t index);

	const Type* GetType(size_t index) const;

	RTTR_REGISTRATION_FRIEND;
private:
	std::vector<Type> GetTypes() const { return m_Types; }
	void SetTypes(std::vector<Type> vTypes);

	std::vector<Type> m_Types = {};
	size_t m_CurrentType = 0;

	float m_fBulletSpeed = 200.f;

	float m_fBulletSpeedDashMultiplier = 1.75f;

	float m_fMinimalBulletSpeed = 0.1f;

	float m_fBulletVoxelHeight = 1.0f;

	RTTR_ENABLE(Entity);
};
