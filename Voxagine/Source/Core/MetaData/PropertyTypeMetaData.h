#pragma once

/*	Enum defining the access modifier for properties and methods
	Public = RTTR registered, visible and modifiable
	Protected = RTTR registered, visible and unmodifiable
	Private = RTTR registered, invisible and unmodifiable */
enum RttrAccessModifier
{
	RAM_PUBLIC,
	RAM_PROTECTED,
	RAM_PRIVATE,
};

/* Macro for defining the access modifiable keyword for properties and methods for RTTR */
#define RTTR_ACCESS_MODIFIER "RttrAccessModifier"

/* Macro for defining a RTTR property or method to be public */
#define RTTR_PUBLIC rttr::metadata(RTTR_ACCESS_MODIFIER, RttrAccessModifier::RAM_PUBLIC)
/* Macro for defining a RTTR property or method to be protected */
#define RTTR_PROTECTED rttr::metadata(RTTR_ACCESS_MODIFIER, RttrAccessModifier::RAM_PROTECTED)
/* Macro for defining a RTTR property or method to be private */
#define RTTR_PRIVATE rttr::metadata(RTTR_ACCESS_MODIFIER, RttrAccessModifier::RAM_PRIVATE)

/*	Enum defining the type for properties
	Resource = property type for resources */
enum RttrPropertyType
{
	RPT_RESOURCE,
};

/*	Macro for defining a RTTR resource 
	X = file extension in std::string ( Example: "file" ) */
#define RTTR_RESOURCE(X) rttr::metadata(RttrPropertyType::RPT_RESOURCE, X)

/**
 * @brief - Macro for defining a RTTR category
 * @param X - the category of the property --> "Value"
 */
#define RTTR_CATEGORY(X) rttr::metadata("Category", X)
/**
 * @brief - Macro for defining a RTTR description
 * @param X - the description of the property --> "Value"
 */
#define RTTR_DESCRIPTION(X) rttr::metadata("Description", X)
/**
 * @brief - Macro for defining a RTTR description
 * @param X - the description of the property --> "Value"
 */
#define RTTR_TOOLTIP(X) rttr::metadata("Tooltip", X)