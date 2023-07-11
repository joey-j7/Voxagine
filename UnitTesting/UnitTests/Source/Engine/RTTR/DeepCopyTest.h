#pragma once

/* RTTR Deep Copy Test error message macro */
#define DeepCopyTestComponentErrorMessage(RTTRTYPE)													\
"RTTR DeepCopy test failed on custom Component Type: " + RTTRTYPE.get_name().to_string() + "!"		\
+ "\nRTTR type size missmatch, register all the properties with RTTR!" ;

/* RTTR Deep Copy Test on given Component CLASS Type*/
#define DEEPCOPYTESTCOMPONENT(COMPONENTTYPE)																		\
TEST(RTTR_DeepCopyTest_Component, COMPONENTTYPE) {																	\
																													\
	/* Create instance of the class which should be tested to force the compiler to run over the pre-compiled */	\
	/* Dummy entities avoids nullptr crashes at the constructor and destructor */									\
	Entity dummyEntity(nullptr);																					\
	COMPONENTTYPE targetDerComponent(&dummyEntity);																	\
																													\
	rttr::type targetDerComponentType = rttr::type::get<COMPONENTTYPE>();											\
	rttr::array_range<rttr::property> targetDerComponentProps =														\
		targetDerComponentType.get_properties(rttr::filter_item::declared_only);									\
																													\
	size_t componentSize = targetDerComponentType.get_sizeof();														\
	size_t componentPropsSize = 0;																					\
																													\
	/* Run the deep copy test on the custom Component (Derived from Component) */									\
	/* Calculates the sum of all the property sizes of the given Component Class */									\
	for (const rttr::property& componentProp : targetDerComponentProps)												\
		componentPropsSize += componentProp.get_type().get_sizeof();												\
																													\
	/* GoogleTest to check if the Component Size matches the calculated sum of all the properties */				\
	EXPECT_EQ(componentSize, componentPropsSize) << DeepCopyTestComponentErrorMessage(targetDerComponentType)		\
};

/* RTTR Deep Copy Test on given Entity CLASS Type*/
#define DEEPCOPYTESTENTITY(ENTITYTYPE)																				\
TEST(RTTR_DeepCopyTest_Entity, ENTITYTYPE) {																		\
																													\
	/* Create instance of the class which should be tested to force the compiler to run over the pre-compiled */	\
	ENTITYTYPE targetDerEntity(nullptr);																			\
																													\
	rttr::type targetDerEntityType = rttr::type::get<ENTITYTYPE>();													\
	rttr::array_range<rttr::property> targetDerEntityProps =														\
		targetDerEntityType.get_properties(rttr::filter_item::declared_only);										\
																													\
	size_t entitySize = targetDerEntityType.get_sizeof();															\
	size_t entityPropsSize = 0;																						\
																													\
	/* Run the deep copy test on the custom Entity (Derived from Entity) */											\
	/* Calculates the sum of all the property sizes of the given Entity Class */									\
	for (const rttr::property& entityProp : targetDerEntityProps)													\
		entityPropsSize += entityProp.get_type().get_sizeof();														\
																													\
	/* GoogleTest to check if the Entity Size matches the calculated sum of all the properties */					\
	EXPECT_EQ(entitySize, entityPropsSize) << DeepCopyTestComponentErrorMessage(targetDerEntityType)				\
};