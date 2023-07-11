#pragma once
#include <queue>
#include <map>
#include "Core/ECS/Components/BehaviorScript.h"
#include "Core/ECS/World.h"

class PoolManager : public BehaviorScript {
public:
	PoolManager(Entity* pEntity) : BehaviorScript(pEntity) {}
	virtual ~PoolManager() = default;
	
	void CreatePool(Entity* pEntity, int iPoolSize) {
		int poolKey = pEntity->GetId();

		if(m_PoolDictionary.find(poolKey) == m_PoolDictionary.end()) {
			m_PoolDictionary.insert({ poolKey, std::queue<Entity*>() });

			for (int i = 0; i < iPoolSize; i++) {
				auto entity = GetWorld()->SpawnEntity<Entity>(GetOwner()->GetTransform()->GetPosition(), Vector3(0.0f), Vector3(1.f));
				entity->SetEnabled(false);
				m_PoolDictionary[poolKey].push(entity);
			}
		}
	}

	void ReuseObject(Entity* prefab, Vector3 position, Quaternion rotation) {
		const int poolKey = prefab->GetId();
		if (m_PoolDictionary.find(poolKey) != m_PoolDictionary.end()) {
			Entity* entity = m_PoolDictionary[poolKey].front();
			m_PoolDictionary[poolKey].pop();
			m_PoolDictionary[poolKey].push(entity);

			entity->SetEnabled(true);
			entity->GetTransform()->SetPosition(position);
			entity->GetTransform()->SetRotation(rotation);
		}
	}
private:
	std::map<int, std::queue<Entity*>> m_PoolDictionary;
};
