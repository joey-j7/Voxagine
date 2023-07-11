#pragma once
#include "Core/Math.h"
#include <vector>

class Chunk;
class ChunkUpdateGroup
{
public:
	enum class UpdateState
	{
		US_INIT,
		US_WAIT,
		US_RENDERING,
		US_UNLOADING
	};

	struct Item
	{
		enum class Target
		{
			T_ASYNC_LOAD,
			T_LOAD,
			T_ASYNC_UNLOAD,
			T_MOVE
		};

		Item(Target _target, Chunk* _pChunk, UVector2 _gridIndex, bool bDone = true) :
			ItemTarget(_target),
			pChunk(_pChunk),
			GridTargetIndex(_gridIndex),
			bIsDone(bDone) {}

		Target ItemTarget;
		bool bIsDone = true;
		Chunk* pChunk = nullptr;
		UVector2 GridTargetIndex = UVector2(0, 0);
	};

	ChunkUpdateGroup(uint32_t gridTargetIndex, Vector3 worldOffset);
	~ChunkUpdateGroup() {};

	UpdateState GetState() const { return m_updateState; }
	void SetState(UpdateState state) { m_updateState = state; }

	void AddItem(ChunkUpdateGroup::Item newItem);
	std::vector<Item>& GetItems() { return m_UpdateItems; }

	uint32_t GetId() const { return m_uiUpdateId; }
	Vector3 GetWorldOffset() const { return m_worldOffset; }

	bool IsChunkScheduledFor(Chunk* pChunk, Item::Target target);
	bool IsRendering() const { return m_bRendering; }
	void SetRendering(bool bRendering) { m_bRendering = bRendering; }

	inline bool operator()(const ChunkUpdateGroup& group) const { return group.m_uiUpdateId == m_uiUpdateId; }

private:
	std::vector<Item> m_UpdateItems;
	UpdateState m_updateState = UpdateState::US_INIT;
	uint32_t m_uiUpdateId = UINT32_MAX;
	Vector3 m_worldOffset = Vector3(0);
	bool m_bRendering = false;
};