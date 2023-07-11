#pragma once

#include "VClass.h"

#include "Core/Utils/Utils.h"

/**
 * @brief TSubclass - This class is intended to show the classes inside the editor
 * That way RTTR knows more about inherit types
 * 
 * ------------------------------------------------------------------
 * \code{.cpp}
 * // Creating objects which is non-component or non-entity and should be of Type subclass
 * TSubclass<Base> prop = TSubclass<Base>();
 * 
 * // Set the inheritanceType
 * prop.SetInstantiateType(rttr::type::get<Monster>());
 * 
 * // Spawn it in the world if it is of type entity
 * prop = GetWorld()->SpawnEntity<Humanoid>(prop, Vector3(0.0f), Quaternion(Vector3(0.0f)), Vector3(1.0));
 * 
 * // Spawn a class that does not need any parameters --> TODO support parameters
 * // Remark don't create a base with a total different inheritance type.
 * prop = TSubClass<Base>::Instantiate(rttr::type<BaseType/InheritanceType>());
 * 
 * // Use the reference immediately
 * prop->(access things inside thr pointer)
 * 
 * \endcode
 * 
 * \see TSubclass
 *
 * -------------------------------------------------------------------
 *
 */
template <typename TClass>
class TSubclass : public VClass
{
public:
	friend class World;
#ifdef EDITOR
	friend class PropertyRenderer;
	friend class JsonSerializer;
#endif

	/**
	 * @brief - This static class with instantiate the class reference inside for you.
	 * Remember though you should not instantiate an entity or a component with this function,
	 * because it needs arguments and also needs to be added to the world. So instead there is a function
	 * inside the world which handles that.
	 * 
	 * @param TargetType - Target type where you want to instantiate to.
	 * @return TSubclass<TClass>
	 *
	 */
	static TSubclass<TClass> Instantiate(const rttr::type& TargetType = rttr::type::get<TClass>());

	/**
	 * @brief - Create a TSubclass with a derived type or else base type
	 * 
	 * @param TargetType - Target type you want
	 */
	TSubclass(const rttr::type& TargetType = rttr::type::get<TClass>());

	/**
	 * @brief - Copy constructor
	 * Use this is want to store a subclass somewhere else.
	 * 
	 * @param From - TSubclass to copy
	 */
	TSubclass(const TSubclass<TClass>& From) : Base(From)
	{
		// only make if class reference exists and if the instantiate type is not a stack member
		if (From.m_pClassReference && rttr::type::get_by_name(m_InstantiateType).get_constructor().get_instantiated_type().is_pointer())
		{
			m_pClassReference = rttr::type::get_by_name(m_InstantiateType).create().template get_value<TClass*>();
		}
	}

	// TSubclass(World* pWorld); // if we want to construct a entity
	// TSubclass(Entity* pEntity); // if we want to construct a component;

	/**
	 * @brief - You can also pass in a pointer from the other TSubclass
	 * which might be helpful.
	 * 
	 * @param From - TSubclass you want to assign from.
	 */
	TSubclass<TClass>& operator=(const TSubclass<TClass>& From)
	{
		Base::operator=(From);
		if (&From == this)
			return *this;

		// reuse storage when possible
		if (m_pClassReference != From.m_pClassReference)
		{
			m_pClassReference = From.m_pClassReference;
		}

		return *this;
	}

	/**
	 * @brief - Immediately assign a pointer to the TSubclass.
	 * 
	 * @param From - pointer to set from.
	 */
	TSubclass<TClass>& operator=(TClass* From)
	{
		// TODO I probably should create a new pointer instead of using the other one.
		std::swap(m_pClassReference, From);
		From = nullptr;
		return *this;
	}

	TClass* operator->() const { return m_pClassReference; }

	/**
	 * @brief - Immediately use the pointer inside the TSubclass
	 * 
	 */
	template<typename Derived>
	Derived* operator->() const { static_assert(std::is_base_of<TClass, Derived>::value, "Derived must be of type TClass"); return static_cast<Derived*>(m_pClassReference); }

	/**
	 * @brief Grab the object inside the TSubclass. Helpful
	 * if you want to nullptr check first and then use it.
	 */
	TClass* GetDefaultObject() { return m_pClassReference; }

	/**
	 * @brief Set the instantiate type. The reason why we override
	 * is because it needs to set the local var to the right type which uses the template
	 * class. if the base is Vclass then it does not matter
	 * 
	 * @param sNewType - New rttr type.
	 */
	void SetInstantiateType(std::string sNewType) override final;

	/**
	 * @brief Instantiate the base type of the TSubclass/VClass
	 * 
	 * @return bool
	 */
	bool InstantiateType() override;

	/**
	 * @brief Instantiate the derived type of the TSubclas/VClass
	 */
	bool InstantiateDerivedType() override;

	/**
	 * @brief See if the derived type is the same as the inner type.
	 * 
	 * @return bool
	 */
	bool is_same() const override
	{
		return m_rInstantiateType == get_inner_type();
	}

	/**
	 *
	 */
	void set_derived_type(rttr::type) override final;

	rttr::type get_inner_type() const override
	{
		return rttr::type::get<TClass>();
	}

	rttr::array_range<rttr::type> get_derived_types() const override { return rttr::type::get<TClass>().get_derived_classes(); }

	virtual ~TSubclass();

	RTTR_ENABLE(VClass)
	RTTR_REGISTRATION_FRIEND
protected:
	typedef VClass Base;

	TClass* m_pClassReference = nullptr;
};

template <typename TClass>
TSubclass<TClass> TSubclass<TClass>::Instantiate(const rttr::type& TargetType)
{
	// Grab the base class type
	rttr::type CurrentType = rttr::type::get<TClass>();
	// Target type if valid
	CurrentType = (TargetType.is_valid() && CurrentType.is_valid() && CurrentType.is_base_of(TargetType)) ? TargetType : CurrentType;

	// Make a subclass
	TSubclass<TClass> s_SubClass(CurrentType);

	//
	s_SubClass.m_bInstantiate = true;
	s_SubClass.m_rInstantiateType = CurrentType;
	s_SubClass.m_pClassReference = s_SubClass.m_rInstantiateType.create().template get_value<TClass*>();

	return s_SubClass;
}

template <typename TClass>
TSubclass<TClass>::TSubclass(const rttr::type& TargetType)
{
	m_bIsSubClass = true;

	m_rInstantiateType = get_inner_type();
	m_rInstantiateType = (TargetType.is_valid() && m_rInstantiateType.is_valid() && m_rInstantiateType.is_base_of(TargetType)) ? TargetType : m_rInstantiateType;
	m_InstantiateType = TargetType.get_name().to_string();
}

template <typename TClass>
void TSubclass<TClass>::SetInstantiateType(std::string sNewType)
{
	m_rInstantiateType = get_inner_type();
	Base::SetInstantiateType(sNewType);
}


template <typename TClass>
bool TSubclass<TClass>::InstantiateType()
{
	m_pClassReference = get_inner_type().create({}).template get_value<TClass*>();
	if (m_pClassReference)
		return true;

	return false;
}

template <typename TClass>
bool TSubclass<TClass>::InstantiateDerivedType()
{
	m_pClassReference = m_rInstantiateType.create({}).template get_value<TClass*>();
	if (m_pClassReference)
		return true;
	return false;
}

template <typename TClass>
void TSubclass<TClass>::set_derived_type(rttr::type sNewType)
{
	// Current base type
	rttr::type CurrentType = rttr::type::get<TClass>();

	// see if valid and see if new type is derived from base. Also see if it is valid.
	if(CurrentType.is_valid() && CurrentType.is_base_of(sNewType) && sNewType.is_valid())
	{
		m_rInstantiateType = sNewType;
		m_InstantiateType = m_rInstantiateType.get_name().to_string();
		// return;
	}

	// TODO use the logger to say that the type is not of base type of this class.
}

/* template <typename TClass>
TSubclass<TClass>::TSubclass(World* pWorld)
{
	// if we are trying to create an entity or component
	static_assert(std::is_base_of<Entity, TClass>::value, "T must be of type Entity");

	m_InstantiateType = rttr::type::get<TClass>().get_name().to_string();
	rttr::type NewClassType = m_InstantiateType;
	rttr::variant NewEntity = NewClassType.create({ pWorld });

	m_ClassReference = NewEntity.get_value<TClass*>();

#ifdef EDITOR
	if (const auto& pActiveEditorWorld = dynamic_cast<EditorWorld*>(pWorld))
	{
		EditorEntityCommand* pEditorEntityCommand = CreateEntityCreationCommand(m_ClassReference);
		pActiveEditorWorld->GetCommandManager().AddCommand(pEditorEntityCommand);
	}
#else
	pWorld->AddEntity(m_ClassReference);
#endif
}

template <typename TClass>
TSubclass<TClass>::TSubclass(Entity* pEntity)
{
	// if we are trying to create an entity or component
	static_assert(std::is_base_of<Component, TClass>::value, "T must be of type Component");

	m_InstantiateType = rttr::type::get<TClass>().get_name().to_string();
	rttr::type NewComponentType = m_InstantiateType;
	rttr::variant NewComponent = NewComponentType.create({ pEntity });
	m_ClassReference = NewComponent.get_value<TClass>();

#ifdef EDITOR
	EditorComponentCommand* pEditorComponentCommand = CreateComponentCreationCommand(pNewComponent);
	pActiveEditorWorld->GetCommandManager().AddCommand(pEditorComponentCommand);
#else
	pEntity->AddComponent(m_ClassReference);
#endif
}*/

template <typename TClass>
TSubclass<TClass>::~TSubclass()
{
	// Component and entity are resolved by the world
	if(m_pClassReference && !Utils::CheckDerivedType(rttr::type::get<TClass*>(), rttr::type::get<Entity*>()) && !Utils::CheckDerivedType(rttr::type::get<TClass*>(), rttr::type::get<Component*>()))
	{
		delete m_pClassReference;
		m_pClassReference = nullptr;
		m_InstantiateType = rttr::type::get<nullptr_t>().get_name().to_string();
	} 
}