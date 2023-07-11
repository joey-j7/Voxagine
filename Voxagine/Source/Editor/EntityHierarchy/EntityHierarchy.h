#pragma once

#include "Editor/Window.h"

#include <string>
#include <vector>

class Entity;

class EntityHierarchy : public Window
{
private:
	struct EntitySearchNode
	{
		EntitySearchNode(Entity* pTargetEntity, EntitySearchNode* pParentNode) : TargetEntity(pTargetEntity), ParentNode(pParentNode) {};

		Entity* TargetEntity = nullptr;
		EntitySearchNode* ParentNode = nullptr;
		
		bool RenderCurrentEntity = false;
		bool RenderAnyChildEntities = false;

		std::vector<EntitySearchNode> ChildNodes;
	};

public:
	EntityHierarchy();
	virtual ~EntityHierarchy();

	virtual void Initialize(Editor* pTargetEditor);
	virtual void UnInitialize();

	virtual void OnContextResize(uint32_t a_uiWidth, uint32_t a_uiHeight, IVector2 deltaResolution) override;

	virtual void Tick(float fDeltaTime);

protected:
	void OnPreRender(float fDeltaTime) override;
	void OnRender(float fDeltaTime) override;

private:
	void OnSearch();

	void RenderCustomToolBar();
	void RenderPopUpWindow();
	void RenderPopUpEntityWindow(Entity* pEntity);
	void RenderEntityHierarchy();

	void RenderEntity(Entity* pRootEntity, Entity* pSelectedEntity);
	void RenderChildEntities(Entity* pTargetEntity, Entity* pSelectedEntity);
	void ProcessEntityDragAndDrop(Entity* pTargetEntity);

	void RenderEntitySearch(EntitySearchNode& rEntitySearchNode, Entity* pSelectedEntity);
	void RenderEntityChildrenSearch(EntitySearchNode& rEntitySearchNode, Entity* pSelectedEntity);
	void EntityNeedsToBeRendererd(Entity* pTargetEntity, EntitySearchNode& rEntitySearchNode, const std::string& NameSearch);

	bool IsCurrentTreeNodeClicked();
	std::string GenerateTreeNodeLabel(Entity* pTargetEntity);
	std::string GenerateSearchTreeNodeLabel(Entity* pTargetEntity);
private:
	std::string m_SearchString;
	std::string m_TempSearchString;
	std::string m_AddCustomEntitySearchString;
	long long m_UniqueSearchID = 0;

	Entity* m_pSelectedEntity = nullptr;
	bool bClickOnEntity = false;
};