#pragma once
#include <unordered_map>
#include "Core/Event.h"
#include "Core/Threading/Job.h"

#pragma warning(push, 0) 
#include <External/moodycamel/readerwriterqueue.h>
#include <External/moodycamel/concurrentqueue.h>
#pragma warning(pop) 

#include "Core/Math.h"

class VoxelGrid;
struct Voxel;
class IntegrityJob : public Job
{
public:
	Event<IntegrityJob*> Stopped;

	void Run() override;
	void Finish() override;
	void Canceled() override;

	moodycamel::ReaderWriterQueue<std::vector<uint64_t>>& GetResults() { return m_Results; }

	//Performs flood fill algorithm that breaks voxels into particles which are not attached to the ground.
	//This function can be very heavy when a large number of voxels have to be searched
	void IntegrityCheck(const Vector3& gridPos);

	void SetVoxelGrid(const VoxelGrid* pGrid) { m_pVoxelGrid = pGrid; }
	void Stop();

	void EnqueueBulk(std::vector<uint64_t>& checks);

private:
	std::atomic<bool> m_bExit = { false };

	const VoxelGrid* m_pVoxelGrid = nullptr;
	moodycamel::ConcurrentQueue<uint64_t> m_IntegrityCheckQueue;
	moodycamel::ReaderWriterQueue<std::vector<uint64_t>> m_BulkCheckQueue;
	moodycamel::ReaderWriterQueue<std::vector<uint64_t>> m_Results;
};