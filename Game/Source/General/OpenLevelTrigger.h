#pragma once
#include <Core/ECS/Entity.h>

class AudioContext;
class AudioSource;

class OpenLevelTrigger : public Entity
{
public:

	OpenLevelTrigger(World* world);

	void Awake() override;
	void Start() override;

	void OnCollisionEnter(Collider* pCollider, const Manifold& manifold) override;
	void Tick(float fDeltaTime) override;

	static void UnlockWorlds(std::vector<std::string> m_sWorldPaths);

	float GetFadeTime() const { return m_fFadeTime; }
	void SetFadeTime(float fFadeTime);

private:
	std::string m_sNextWorldFilepath = "Content/Worlds/Menus/Main_Menu.wld";

	AudioContext* m_pAudioContext = nullptr;
	AudioSource* m_pAudioSource = nullptr;

	std::vector<std::string> m_sUnlockingWorlds;

	float m_fFadeTime = 3.f;
	float m_fFadeTimer = 0.f;

	float m_fBGMVolume = 1.f;

	bool m_bIsTriggered;
	bool m_bStopMusic = false;

	RTTR_ENABLE(Entity)
	RTTR_REGISTRATION_FRIEND
};