#include "pch.h"
#include "Core/ECS/Systems/Chunk/ChunkUpdateGroup.h"

ChunkUpdateGroup::ChunkUpdateGroup(uint32_t gridTargetIndex, Vector3 worldOffset)
{
	m_uiUpdateId = gridTargetIndex;
	m_worldOffset = worldOffset;
}

void ChunkUpdateGroup::AddItem(Item newItem)
{
	for (Item& item : m_UpdateItems)
	{
		if (item.pChunk == newItem.pChunk)
			return;
	}

	m_UpdateItems.push_back(newItem);
}

bool ChunkUpdateGroup::IsChunkScheduledFor(Chunk* pChunk, Item::Target target)
{
	for (Item& item : m_UpdateItems)
	{
		if (item.pChunk == pChunk)
		{
			return item.ItemTarget == target;
		}
	}
	return false;
}