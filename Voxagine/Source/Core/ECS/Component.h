#pragma once
#include <assert.h>
#include "Core/Event.h"
#include "Core/ECS/Entity.h"

#include <External/rttr/type>
#include <External/rttr/registration_friend> 

class Transform;
class Component
{
public:
	friend class Entity;

	Component(Entity* pOwner);
	virtual ~Component() {}

	Event<Component*> Destroyed;

	virtual void Awake() {};
	virtual void Start() {};

	virtual void OnEnabled() {}
	virtual void OnDisabled() {}
	
	World* GetWorld() const { return m_pOwner->GetWorld(); }
	Transform* GetTransform() const { return m_pTransform; }
	Entity* GetOwner() const { return m_pOwner; }
	bool IsInitialized() { return m_bIsInitialized; }
	bool IsComponentOnlyEnabled() const { return IsEnabled(false); };
	bool IsEnabled(bool bWithParent = true) const;
	void SetEnabled(bool bEnabled);

	bool HasTag(std::string tag);

	//Returns a reference of the tags vector
	std::vector<std::string>& GetTags() { return m_Tags; }

	//Performs a dependency check on this component regarding the given component, component will be disabled if dependencies are not met
	void DependencyCheck(Component* pComponent, bool bAdded);

	template<typename T>
	void Requires();

	bool m_bEnabled;
protected:
	Transform* m_pTransform;

private:
	bool FindDependency(rttr::type depType);

	Entity* m_pOwner;
	bool m_bIsInitialized;
	std::vector<rttr::type> m_Dependencies;
	std::vector<std::string> m_Tags;

	RTTR_ENABLE()
	RTTR_REGISTRATION_FRIEND
};

template<typename T>
void Component::Requires()
{
	m_Dependencies.push_back(rttr::type::get<T>());
}
