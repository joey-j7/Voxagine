#include "DoorPrefab.h"

#include <Core/Application.h>
#include <Core/LoggingSystem/LoggingSystem.h>
#include <Core/ECS/World.h>
#include "Core/ECS/Components/VoxRenderer.h"
#include "Core/ECS/Components/BoxCollider.h"

#include <External/rttr/registration.h>

RTTR_REGISTRATION
{
	rttr::registration::class_<DoorPrefab>("DoorPrefab")
		.constructor<World*>()(
			// rttr::policy::ctor::as_object,
			rttr::policy::ctor::as_raw_ptr // ,
			// rttr::policy::ctor::as_std_shared_ptr
		)
		.property("Level Name", &DoorPrefab::m_LevelName)
		.property("Won", &DoorPrefab::m_bWon)
		.property("Door ResetTimer", &DoorPrefab::m_fWinResetTimer)
		.property_readonly("Door Timer", &DoorPrefab::m_fWinTimer)
	;
}

void DoorPrefab::SetWorld(const std::string& levelName)
{
	m_LevelName = levelName;
}

void DoorPrefab::Start()
{
	Entity::Start();

	VoxModel* pVoxModel = GetWorld()->GetApplication()->GetResourceManager().LoadVox("Content/Models/Props/door.vox");
	auto VoxRenderComp = AddComponent<VoxRenderer>();
	if (!VoxRenderComp)
		VoxRenderComp = GetComponent<VoxRenderer>();

	auto pBoxCollider = AddComponent<BoxCollider>();
	
	if (!pBoxCollider)
		pBoxCollider = GetComponent<BoxCollider>();

	VoxRenderComp->SetModel(pVoxModel);
	pBoxCollider->SetBoxSize(pVoxModel->GetFrame(0));
	pBoxCollider->SetTrigger(true);
}

void DoorPrefab::Tick(float fDeltaTime)
{
	Entity::Tick(fDeltaTime);
	if (m_bWon) {
		m_fWinTimer -= GetWorld()->GetDeltaSeconds();
		if (m_fWinTimer <= 0.0f)
		{
			// GetWorld()->OpenWorld(m_LevelName, true);
			m_bWon = false;
			m_fWinTimer = m_fWinResetTimer;
			Destroy();
		}
	}
}


void DoorPrefab::OnCollisionEnter(Collider* pCollider, const Manifold&)
{
	if (pCollider->GetOwner()->GetName() == "Player") {
		if (!m_bWon) {
			if (!m_LevelName.empty())
			{
				m_bWon = true;
			}
			else
			{
				LogEvent logEvent;
				logEvent.Level = LOGLEVEL_WARNING;
				logEvent.Category = "AI";
				logEvent.Description = "Door.cpp(33): Level name is not set!";
				GetWorld()->GetApplication()->GetLoggingSystem().Log(logEvent);
			}
		}
	}
}

void DoorPrefab::OnCollisionStay(Collider* pCollider, const Manifold&)
{
	if (pCollider->GetOwner()->GetName() == "Player") {
		if (!m_bWon) {
			if (!m_LevelName.empty())
			{
				m_bWon = true;
			}
			else
			{
				LogEvent logEvent;
				logEvent.Level = LOGLEVEL_WARNING;
				logEvent.Category = "AI";
				logEvent.Description = "Door.cpp(33): Level name is not set!";
				GetWorld()->GetApplication()->GetLoggingSystem().Log(logEvent);
			}
		}
	}
}
