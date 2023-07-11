#pragma once
#include <unordered_map>
#include "ReferenceObject.h"

template <typename T>
class ReferenceManager
{
static_assert(std::is_base_of<ReferenceObject, T>::value, "Type must derive from ReferenceObject");

public:
	~ReferenceManager() { ClearAll(); }

	T* AddReference(const std::string& ref);
	void RemoveReference(const std::string& ref);
	T* GetReference(const std::string& ref);
	void ClearAll();

	void GetResourceFilePaths(std::vector<std::string>& ResourceFilePaths, std::string fileExtension = std::string());

private:
	std::unordered_map<std::string, T*> m_References;
};

template <typename T>
void ReferenceManager<T>::ClearAll()
{
	typename std::unordered_map<std::string, T*>::iterator iter = m_References.begin();
	for (; iter != m_References.end(); ++iter)
	{
		if (iter->second)
		{
			delete iter->second;
			iter->second = nullptr;
		}
	}
	m_References.clear();
}

template<typename T>
inline void ReferenceManager<T>::GetResourceFilePaths(std::vector<std::string>& ResourceFilePaths, std::string fileExtension)
{
	bool HasFileExtensionFilter = (fileExtension != std::string());
	if (HasFileExtensionFilter)
	{
		size_t founddot = fileExtension.find(".");
		if (founddot == std::string::npos || founddot != 0)
			fileExtension.insert(0, ".");
	}

	for (auto& it : m_References)
	{
		bool FileExtensionValid = true;

		if (HasFileExtensionFilter)
		{
			std::size_t found = it.first.find(".");

			if (found != std::string::npos)
			{
				if (it.first.substr(found, it.first.length() - found) != fileExtension)
					FileExtensionValid = false;
			}
			else
			{
				FileExtensionValid = false;
			}
		}

		if (FileExtensionValid)
			ResourceFilePaths.push_back(it.first);
	}

	std::sort(ResourceFilePaths.begin(), ResourceFilePaths.end());
}

template<typename T>
inline T* ReferenceManager<T>::AddReference(const std::string& ref)
{
	typename std::unordered_map<std::string, T*>::iterator iter = m_References.find(ref);

	if (iter != m_References.end()) 
	{
		m_References[ref]->IncrementRef();
		return m_References[ref];
	}

	T* pRefObject = new T(ref);
	pRefObject->Released += Event<ReferenceObject*>::Subscriber([this](ReferenceObject* pRefObj) {
		RemoveReference(pRefObj->GetRefPath());
	}, this);
	m_References.emplace(ref, pRefObject);
	return pRefObject;
}

template <typename T>
void ReferenceManager<T>::RemoveReference(const std::string& ref)
{
	typename std::unordered_map<std::string, T*>::iterator iter = m_References.find(ref);

	if (iter != m_References.end())
	{
		iter->second->DecrementRef();
		if (iter->second->GetRefCount() <= 0) {
			delete iter->second;
			iter->second = nullptr;
			m_References.erase(iter);
		}
		return;
	}
}

template<typename T>
inline T* ReferenceManager<T>::GetReference(const std::string& ref)
{
	typename std::unordered_map<std::string, T*>::iterator iter = m_References.find(ref);
	if (iter == m_References.end())
		return nullptr;
	return iter->second;
}