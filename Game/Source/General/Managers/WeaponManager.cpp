#include "WeaponManager.h"

#include <External/rttr/registration>
#include <External/rttr/policy.h>
#include "Core/MetaData/PropertyTypeMetaData.h"

#include "Core/PlayerPrefs/PlayerPrefs.h"

RTTR_REGISTRATION
{

	rttr::registration::enumeration<WeaponManager::Type::MovementType>("Movement Type")
	(
		rttr::value("Straight", WeaponManager::Type::MovementType::MT_STRAIGHT),
		rttr::value("Sine", WeaponManager::Type::MovementType::MT_SINE)
	);

	rttr::registration::enumeration<WeaponManager::Type::ActivatedType>("Activated Type")
	(
		rttr::value("ActiveTest", WeaponManager::Type::ActivatedType::AT_TEST) //,
		// rttr::value("Sine", WeaponManager::Type::ActivatedType::MT_SINE)
	);


	rttr::registration::class_<WeaponManager::Type>("Weapon Type")
		.constructor<>()(rttr::policy::ctor::as_object)
		.property("Movement Type", &WeaponManager::Type::m_MovementType)(RTTR_PUBLIC)
		.property("Activated Type", &WeaponManager::Type::m_ActivatedType)(RTTR_PUBLIC)

		.property("Perfect Catch Range", &WeaponManager::Type::GetPerfectCatchRange, &WeaponManager::Type::SetPerfectCatchRange) (RTTR_PUBLIC)
		.property("Catch Range", &WeaponManager::Type::GetCatchRange, &WeaponManager::Type::SetCatchRange) (RTTR_PUBLIC)
		.property("Auto Catch Range", &WeaponManager::Type::GetAutoCatchRange, &WeaponManager::Type::SetAutoCatchRange) (RTTR_PUBLIC)
		.property("Minimum Perfect Catch Speed", &WeaponManager::Type::GetMinPerfCatchSpeed, &WeaponManager::Type::SetMinPerfCatchSpeed) (RTTR_PUBLIC)

		.property("Max ammo", &WeaponManager::Type::m_uiMaxAmmo)(RTTR_PUBLIC)
		.property("Damage", &WeaponManager::Type::m_fDamage)(RTTR_PUBLIC)
	
		.property("Shooting Delay", &WeaponManager::Type::m_fShootingDelay)(RTTR_PUBLIC)
		.property("Is Current", &WeaponManager::Type::m_bIsCurrentWeapon)(RTTR_PUBLIC)

		.property("Explosion Range", &WeaponManager::Type::m_fBulletExplosionRange)(RTTR_PUBLIC)

		.property("Travel Time", &WeaponManager::Type::m_fTravelTime)(RTTR_PUBLIC)
		.property("Decay Time", &WeaponManager::Type::m_fDecayTime)(RTTR_PUBLIC)

		.property("Rotation Speed", &WeaponManager::Type::m_fRotationSpeed)(RTTR_PUBLIC)

		.property("Animation Amplitude", &WeaponManager::Type::m_fPathAnimationAmplitude)(RTTR_PUBLIC)
		.property("Animation Time", &WeaponManager::Type::m_fPathAnimationTime)(RTTR_PUBLIC)
		.property("Animation Loop Max", &WeaponManager::Type::m_uiPathAnimationLoopMax)(RTTR_PUBLIC)

		.property("Model", &WeaponManager::Type::m_ModelPath)(RTTR_PUBLIC, RTTR_RESOURCE("vox"))
		.property("Icon", &WeaponManager::Type::m_IconPath)(RTTR_PUBLIC, RTTR_RESOURCE("png"))
		.property("Combo 0 Model", &WeaponManager::Type::m_combo0ModelPath)(RTTR_PUBLIC, RTTR_RESOURCE("vox"))
		.property("Combo 1 Model", &WeaponManager::Type::m_combo1ModelPath)(RTTR_PUBLIC, RTTR_RESOURCE("vox"))
		.property("Combo 2 Model", &WeaponManager::Type::m_combo2ModelPath)(RTTR_PUBLIC, RTTR_RESOURCE("vox"))
		.property("Combo 3 Model", &WeaponManager::Type::m_combo3ModelPath)(RTTR_PUBLIC, RTTR_RESOURCE("vox"))
		.property("Unlocked", &WeaponManager::Type::IsUnlocked, &WeaponManager::Type::SetUnlocked)(RTTR_PUBLIC)
		.property("Activated", &WeaponManager::Type::m_bActivated)(RTTR_PUBLIC)
		.property("ResourceAmount", &WeaponManager::Type::m_fResourceAmount)(RTTR_PUBLIC)

		.property("Camera Shake", &WeaponManager::Type::m_fCameraShake)(RTTR_PUBLIC)
		.property("Max Camera Shake", &WeaponManager::Type::m_fMaxCameraShake)(RTTR_PUBLIC)
	;

	rttr::registration::class_<WeaponManager>("Weapon Manager")
		.constructor<World*>()(
			rttr::policy::ctor::as_raw_ptr
		)

		.property("Types", &WeaponManager::GetTypes, &WeaponManager::SetTypes)(RTTR_PUBLIC)
		.property("Current Type", &WeaponManager::GetTypeID, rttr::select_overload<void(size_t)>(&WeaponManager::SetTypeID))(RTTR_PUBLIC)

		.property("Bullet Speed", &WeaponManager::m_fBulletSpeed) (RTTR_PUBLIC)
		.property("Bullet Speed Dash Multiplier", &WeaponManager::m_fBulletSpeedDashMultiplier) (RTTR_PUBLIC)
		.property("Speed Threshold", &WeaponManager::m_fMinimalBulletSpeed) (RTTR_PUBLIC, RTTR_TOOLTIP("Minimal speed of killing the enemies"))
		.property("Minimal Bullet Box Height", &WeaponManager::m_fBulletVoxelHeight) (RTTR_PUBLIC, RTTR_TOOLTIP("Minimal Box height of bumping"))
	;
}

void WeaponManager::Type::SetUnlocked(bool bUnlocked)
{
	if (bUnlocked == m_bUnlocked)
		return;

	m_bUnlocked = bUnlocked;

	// Check if we unlocked the sine movement
	if (m_MovementType >= MovementType::MT_SINE)
	{
		const rttr::enumeration enumType = rttr::type::get(m_MovementType).get_enumeration();
		PlayerPrefs::GetBoolAccessor().Set(enumType.value_to_name(m_MovementType).to_string(), m_bUnlocked);
	}

	if (m_ActivatedType >= ActivatedType::AT_TEST)
	{
		const rttr::enumeration enumType = rttr::type::get(m_ActivatedType).get_enumeration();
		PlayerPrefs::GetBoolAccessor().Set(enumType.value_to_name(m_MovementType).to_string(), m_bUnlocked);
	}

	// NOTE Use this for saving instead of SaveSettings !
	PlayerPrefs::Save();
}

void WeaponManager::Type::Initialize()
{
	// Check if we unlocked the sine movement
	if (m_MovementType >= MovementType::MT_SINE)
	{
		const rttr::enumeration enumType = rttr::type::get(m_MovementType).get_enumeration();
		m_bUnlocked = PlayerPrefs::GetBoolAccessor().Get(enumType.value_to_name(m_MovementType).to_string(), false);

		// if this is not the weapon that has been unlocked put it to STRAIGHT
		if(!m_bUnlocked)
			m_MovementType = MovementType::MT_STRAIGHT;
	}

	if (m_ActivatedType >= ActivatedType::AT_TEST)
	{
		const rttr::enumeration enumType = rttr::type::get(m_ActivatedType).get_enumeration();
		m_bUnlocked = PlayerPrefs::GetBoolAccessor().Get(enumType.value_to_name(m_ActivatedType).to_string(), false);
	}
}

WeaponManager::WeaponManager(World* pWorld) : Entity(pWorld)
{
	//This will be the name of this entity once it is spawned in the world
	SetName("Weapon Manager");
}

void WeaponManager::Awake()
{
	Entity::Awake();

	/* Add default weapon */
	if (m_Types.empty())
	{
		Type weaponType;

		weaponType.m_uiMaxAmmo = 1;
		weaponType.m_bIsCurrentWeapon = true;
		weaponType.SetOwner(this);
		m_Types.push_back(
			weaponType
		);
	} 
	else
	{
		// HOTFIX for grabbing the ID of the entity for the EditorCommand.cpp
		for (auto& type : m_Types)
		{
			if (!type.GetOwner())
				type.SetOwner(this);

			type.Initialize();
		}
	}

	if (!m_Types.empty())
	{
		m_Types[0].m_combo0ModelPath = "Content/Models/Projectiles/Weapon_Blue_Version_1.vox";
		m_Types[0].m_combo1ModelPath = "Content/Models/Projectiles/Weapon_Blue_Version_2.vox";
		m_Types[0].m_combo2ModelPath = "Content/Models/Projectiles/Weapon_Blue_Version_3.vox";
		m_Types[0].m_combo3ModelPath = "Content/Models/Projectiles/Weapon_Blue_Version_4.vox";
	}

	/* Add tag - Addtag will already check if it exists or not. */
	AddTag("WeaponManager");

	SetPersistent(true);
}

void WeaponManager::Start()
{
	Entity::Start();

	if (!m_Types.empty())
	{
		m_Types[0].m_combo0ModelPath = "Content/Models/Projectiles/Weapon_Blue_Version_1.vox";
		m_Types[0].m_combo1ModelPath = "Content/Models/Projectiles/Weapon_Blue_Version_2.vox";
		m_Types[0].m_combo2ModelPath = "Content/Models/Projectiles/Weapon_Blue_Version_3.vox";
		m_Types[0].m_combo3ModelPath = "Content/Models/Projectiles/Weapon_Blue_Version_4.vox";
	}
}

const WeaponManager::Type* WeaponManager::GetCurrentType() const
{
	return m_Types.empty() ? nullptr : &m_Types[m_CurrentType];
}

size_t WeaponManager::GetTypeID() const
{
	return m_CurrentType;
}

void WeaponManager::SetTypeID(size_t index)
{
	if (m_Types.empty())
		return;

	m_Types[m_CurrentType].m_bIsCurrentWeapon = false;
	m_CurrentType = std::min(m_Types.size() - 1, index);
	m_Types[m_CurrentType].m_bIsCurrentWeapon = true;
}

const WeaponManager::Type* WeaponManager::GetType(size_t index) const
{
	return index <= m_Types.size() ? &m_Types[index] : nullptr;
}

void WeaponManager::Type::SetCatchRange(float fCatchRange)
{
	m_fCatchRange = std::max(FLT_MIN, std::max(m_fPerfectCatchRange, fCatchRange));
}

void WeaponManager::Type::SetAutoCatchRange(float fCatchRange)
{
	m_fAutoCatchRange = std::max(FLT_MIN, std::max(m_fCatchRange, fCatchRange));
}

void WeaponManager::Type::SetPerfectCatchRange(float fPerfectCatchRange)
{
	m_fPerfectCatchRange = std::max(FLT_MIN, std::min(m_fCatchRange, fPerfectCatchRange));
}

void WeaponManager::Type::SetMinPerfCatchSpeed(float fMinPerfCatchSpeed)
{
	m_fMinPerfCatchSpeed = std::max(0.f, fMinPerfCatchSpeed);
}

void WeaponManager::SetTypes(std::vector<Type> vTypes)
{
	if (!vTypes.empty())
	{
		auto& type = vTypes.back();
		if (!type.GetOwner())
			type.SetOwner(this);
	}

	m_Types = std::move(vTypes);
}

