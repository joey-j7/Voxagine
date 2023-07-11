#pragma once
#include <External/rttr/type>
#include <External/rttr/registration_friend> 

class WorldCreateConfig
{
public:
	uint32_t GetWorldChunkSize() const { return m_uiChunkWorldSize; }
	void SetWorldChunkSize(uint32_t size);

	uint32_t GetMaxParticles() const { return m_uiMaxParticles; }
	void SetMaxParticles(uint32_t count) { m_uiMaxParticles = count; }

private:
	uint32_t m_uiChunkWorldSize = 1;
	uint32_t m_uiMaxParticles = 150000;

	RTTR_ENABLE();
	RTTR_REGISTRATION_FRIEND
};