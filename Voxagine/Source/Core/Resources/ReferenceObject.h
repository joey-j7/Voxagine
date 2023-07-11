#pragma once
#include <string>
#include "Core/Event.h"
#include "Core/System/FileSystem.h"

#include <stdint.h>

class ReferenceObject
{
public:
	template<typename T>
	friend class ReferenceManager;

	ReferenceObject(std::string refPath) { m_refPath = refPath; }
	virtual ~ReferenceObject() {}

	Event<ReferenceObject*> Released;

	void SetFileSystem(FileSystem* pFileSystem) { m_pFileSystem = pFileSystem; }

	/* Releases the resource for this instance, call this function once you are done with a resource */
	void Release() { Released(this); }

	virtual void IncrementRef() { ++m_uiRefCount; }
	
	std::string GetRefPath() const { return m_refPath; }
	uint32_t GetRefCount() { return m_uiRefCount; }

	bool IsLoaded() const { return m_bIsLoaded; }

	virtual bool Load(const std::string& filePath) = 0;

protected:
	virtual void Free() = 0;
	bool m_bIsLoaded = false;
	FileSystem* m_pFileSystem = nullptr;

private:
	virtual void DecrementRef() { --m_uiRefCount; }

	std::string m_refPath = "";
	uint32_t m_uiRefCount = 1;
};