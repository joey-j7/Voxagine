#include "pch.h"
#include "Core/ECS/Systems/Physics/IntegrityJob.h"

#include "Core/ECS/Systems/Physics/VoxelGrid.h"
#include <stack>
#include <map>
#include <thread>

void IntegrityJob::Run()
{
	if (m_pVoxelGrid == nullptr)
		return;

	while (!m_bExit.load())
	{
		std::vector<uint64_t> bulk;
		if (m_BulkCheckQueue.try_dequeue(bulk))
		{
			std::sort(bulk.begin(), bulk.end());
			bulk.erase(std::unique(bulk.begin(), bulk.end()), bulk.end());
			m_IntegrityCheckQueue.enqueue_bulk(&bulk[0], bulk.size());
		}

		uint64_t vecHash;
		if (m_IntegrityCheckQueue.try_dequeue(vecHash))
		{
			Vector3 vec;
			vec.x = ((uint16_t*)&vecHash)[2];
			vec.y = ((uint16_t*)&vecHash)[1];
			vec.z = ((uint16_t*)&vecHash)[0];
			IntegrityCheck(vec);
		}
		else
		{
			std::this_thread::sleep_for(std::chrono::milliseconds(10));
		}
	}
}

void IntegrityJob::Finish()
{
	Stopped(this);
}

void IntegrityJob::Canceled()
{
	m_bExit.store(true);
}

void IntegrityJob::IntegrityCheck(const Vector3& gridPos)
{
	std::unordered_map<uint64_t, Vector3> checkedVoxels;
	std::stack<Vector3> checkStack;
	checkStack.push(gridPos);

	while (!checkStack.empty() && !m_bExit.load())
	{
		Vector3 pos = checkStack.top();
		const Voxel* pVoxel = m_pVoxelGrid->GetVoxel((uint32_t)pos.x, (uint32_t)pos.y, (uint32_t)pos.z);
		checkStack.pop();

		uint64_t hash = 0;
		((uint16_t*)&hash)[0] = (uint16_t)pos.z;
		((uint16_t*)&hash)[1] = (uint16_t)pos.y;
		((uint16_t*)&hash)[2] = (uint16_t)pos.x;

		std::unordered_map<uint64_t, Vector3>::iterator iter = checkedVoxels.find(hash);
		if (iter != checkedVoxels.end())
			continue;

		bool isVoxelActive = (pVoxel && pVoxel->Active);
		if (pos.y - 1 == 0 && isVoxelActive)
			return;

		if (isVoxelActive)
		{
			checkedVoxels[hash] = pos;

			for (int y = 1; y >= -1; --y)
			{
				for (int x = -1; x <= 1; ++x)
				{
					for (int z = -1; z <= 1; ++z)
					{
						if (x == 0 && y == 0 && z == 0) continue;

						checkStack.push(pos + Vector3(
							static_cast<float>(x),
							static_cast<float>(y),
							static_cast<float>(z)));
					}
				}
			}
		}
	}

	if (checkedVoxels.size() > 0 && !m_bExit.load())
	{
		std::vector<uint64_t> checkedPositions;
		checkedPositions.reserve(checkedVoxels.size());

		for (auto& hashVec : checkedVoxels)
			checkedPositions.push_back(hashVec.first);

		std::sort(checkedPositions.begin(), checkedPositions.end());

		m_Results.enqueue(checkedPositions);
		return;
	}
	return;
}

void IntegrityJob::Stop()
{
	m_bExit.store(true);
}

void IntegrityJob::EnqueueBulk(std::vector<uint64_t>& checks)
{
	if (checks.size() > 0)
	{
		m_BulkCheckQueue.enqueue(checks);
	}
}