#pragma once

#include <External/rttr/registration>
#include <External/rttr/registration_friend>

class Entity;

/**
 * @brief VClass - This class is intended to show a struct/class inside the editor.
 * From there you can adjust properties and use it during gameplay.
 *  
 * ------------------------------------------------------------------
 * 
 * \code{.cpp}
 * class Derived : public VClass
 * {
 * }
 * 
 * // Creating objects which is non-component or non-entity
 * Derived prop; // prop will show up in the editor now.
 * 
 * 
 * \endcode
 * 
 * \see TSubclass
 *
 */
class VClass
{
public:
	friend class World;

	VClass();
	VClass(const VClass& From)
	{
		m_bIsSubClass = From.m_bIsSubClass;
		m_bInstantiate = From.m_bInstantiate;
		m_rInstantiateType = From.m_rInstantiateType;
		m_InstantiateType = From.m_InstantiateType;
		m_pEntity = From.m_pEntity;
	}

	virtual ~VClass() = default;

	VClass& operator=(const VClass& From)
	{
		if (&From == this)
			return *this;

		m_bIsSubClass = From.m_bIsSubClass;
		m_bInstantiate = From.m_bInstantiate;
		m_rInstantiateType = From.m_rInstantiateType;
		m_InstantiateType = From.m_InstantiateType;
		m_pEntity = From.m_pEntity;

		return *this;
	}

	VClass& operator=(std::string From)
	{
		std::swap(m_InstantiateType, From);
		// Also set the new type
		m_rInstantiateType = rttr::type::get_by_name(m_InstantiateType);
		return *this;
	}

	void SetOwner(Entity* pEntity) { m_pEntity = pEntity; }
	Entity* GetOwner() const { return m_pEntity; }

	bool ShouldInstantiate() const { return m_bInstantiate; }

	virtual void SetInstantiateType(std::string);
	std::string GetInstantiateType() const;
	
	bool is_sub_class() const;

	virtual bool InstantiateType() { return false; }
	virtual bool InstantiateDerivedType() { return false; }
	
	// needs to be overriden to see if the string type is the same as the other type.
	virtual void set_derived_type(rttr::type);
	virtual bool is_same() const { return false; }
	virtual rttr::type get_derived_type() const { return m_rInstantiateType; }
	virtual rttr::type get_inner_type() const { return m_rInstantiateType; }
	virtual rttr::array_range<rttr::type> get_derived_types() const { return  m_rInstantiateType.get_derived_classes(); };

	RTTR_ENABLE()
	RTTR_REGISTRATION_FRIEND
protected:
#ifdef EDITOR
	friend class PropertyRenderer;
	friend class JsonSerializer;
#endif
	bool m_bIsSubClass = false;
	bool m_bInstantiate = false;
	rttr::type m_rInstantiateType = rttr::type::get<VClass>();
	std::string m_InstantiateType = m_rInstantiateType.get_name().to_string();

	Entity* m_pEntity = nullptr;
};
