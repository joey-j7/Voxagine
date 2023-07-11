#include "Weapon.h"

#include <Core/ECS/World.h>

#include "Bullet.h"
#include "Humanoids/Players/Player.h"

#include <External/rttr/registration>
#include <External/rttr/policy.h>

// #include <External/rttr/registration>
// #include <External/rttr/policy.h>
#include "General/Managers/WeaponManager.h"
#include "Core/MetaData/PropertyTypeMetaData.h"

RTTR_REGISTRATION
{
	//Register the weapon to get the weapon name from the Enum (inside GetWeaponName) and then set it as the new weapon
	rttr::registration::class_<Weapon>("Weapon")
		.constructor<Entity*>()(rttr::policy::ctor::as_raw_ptr)
		.property("Ammo", &Weapon::m_uiAmmo)(RTTR_PUBLIC)
		.property("Unlimited Ammo", &Weapon::m_bUnlimitedAmmo)(RTTR_PUBLIC)
		.property("Spawn Offset", &Weapon::m_SpawnOffset)(RTTR_PUBLIC);
}

//This is the constructor for the weapon class
Weapon::Weapon(Entity* pOwner) :
	Component(pOwner)
{

}

void Weapon::Start()
{
	Component::Start();

	m_pPlayer = dynamic_cast<Player*>(GetOwner());

	std::vector<Entity*> pManagers = GetWorld()->FindEntitiesWithTag("WeaponManager");
	const auto vManagers = GetWorld()->FindEntitiesOfType<GameManager>();
	if(!vManagers.empty())
		m_pGameManager = vManagers[0];

	if (!pManagers.empty())
		m_pManager = dynamic_cast<WeaponManager*>(pManagers.front());
	else
		m_pManager = GetWorld()->SpawnEntity<WeaponManager>(Vector3(), Quaternion(), Vector3());
}

uint32_t Weapon::GetCurrentAmmo() const
{
	return m_uiAmmo;
}

void Weapon::SetCurrentAmmo(uint32_t uiAmmo)
{
	if (!GetType()) return;
	m_uiAmmo = std::min(GetType()->m_uiMaxAmmo, uiAmmo);
}

bool Weapon::HasInfiniteAmmo() const
{
	return m_bUnlimitedAmmo;
}

const WeaponManager::Type* Weapon::GetType() const
{
	return m_pManager ? m_pManager->GetCurrentType() : nullptr;
}

void Weapon::ResetAmmo()
{
	if (!GetType()) return;
	m_uiAmmo = GetType()->m_uiMaxAmmo;
}

void Weapon::Fire()
{
	if ((m_uiAmmo == 0 && !m_bUnlimitedAmmo) || !m_pPlayer || !m_pManager || !m_pGameManager)
		return;

	currentComboTimer = m_pGameManager->GetComboTimer();

	if (Utils::InRange((m_pGameManager->comboTimerStartTime - m_pGameManager->Bonus1TimeHeld), currentComboTimer, m_pGameManager->comboTimerStartTime))
	{
		m_pGameManager->AddComboStreak(m_pGameManager->timeBasedBonus1);
	}
	else if (Utils::InRange((m_pGameManager->comboTimerStartTime - m_pGameManager->Bonus2TimeHeld), currentComboTimer, (m_pGameManager->comboTimerStartTime - m_pGameManager->Bonus1TimeHeld)))
	{
		m_pGameManager->AddComboStreak(m_pGameManager->timeBasedBonus2);
	}
	else if (currentComboTimer <= 0)
	{
		m_pGameManager->ResetComboStreak();
	}

	const Vector3 v3Direction = m_pPlayer->GetDirection();

	const float fAngle = atan2(v3Direction.x, v3Direction.z);
	const Quaternion quat(std::cos(fAngle * 0.5f), 0.0f, std::sin(fAngle * 0.5f), 0.0f);

	// quat.x = 0.f;
	// quat.y = std::sin(fAngle * 0.5f);
	// quat.z = 0.f;
	// quat.w = std::cos(fAngle * 0.5f);

	auto pBullet = GetWorld()->SpawnEntity<Bullet>(m_pPlayer->GetTransform()->GetPosition() + glm::rotate(quat, m_SpawnOffset), quat, Vector3(1.f));

	pBullet->m_fCameraShake = m_pManager->GetCurrentType()->m_fCameraShake;
	pBullet->m_fMaxCameraShake = m_pManager->GetCurrentType()->m_fMaxCameraShake;

	// It needs to stay alive in every chunk.
	pBullet->SetPersistent(true);

	// Set the caster
	pBullet->SetCurrentCaster(m_pPlayer);

	// Set the receiver
	const auto pLinkedPlayer = m_pPlayer->GetLinkedPlayer();
	pBullet->SetCurrentReceiver(pLinkedPlayer);

	m_pPlayer->AddSpawnedBullet(pBullet);

	pBullet->SetSpeed(m_pManager->m_fBulletSpeed * (m_pPlayer->IsDashing() ? m_pManager->m_fBulletSpeedDashMultiplier : 1.f));
	pBullet->SetDamageAmount(m_pManager->GetCurrentType()->m_fDamage);

	//Calculate if the escape procedure is necessary
	const float fDist = glm::distance(pLinkedPlayer->GetTransform()->GetPosition(), pBullet->GetTransform()->GetPosition());
	const float fAutoRange = pBullet->GetAutoCatchRange();
	const float fRange = pBullet->GetCatchRange();
	const float fEscapeRange = pBullet->GetEscapeRange();

	//The receiver stood to close to the throwing player, start escape procedure
	if (fDist < fAutoRange + fEscapeRange || fDist < fRange + fEscapeRange){
		pBullet->SetEscaped(false);
		pBullet->SetEscapePosition(pLinkedPlayer->GetTransform()->GetPosition());
	}

	m_uiAmmo--;

	if (!m_bUnlimitedAmmo && m_uiAmmo == 0)
		m_pPlayer->HideAimer();
}