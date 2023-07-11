#pragma once

#include <vector>
#include "Core/Math.h"

namespace rttr 
{
	class type;
	class property;
}

namespace Utils 
{
	static const int FULL_CIRCLE_RADIANS = 360; // * 0.0174532925f;

	/*!
	 * @brief - Take the average of the vector
	 * 
	 * @param vector
	 * @param iCount
	 */
	inline void Average(Vector3& vector, int iCount) 
	{
		vector.x /= static_cast<float>(iCount);
		vector.y /= static_cast<float>(iCount);
		vector.z /= static_cast<float>(iCount);
	}

	template<typename T>
	std::initializer_list<T>& ToInitializerList(std::vector<T> V)
	{
		return std::initializer_list<T>(V.data(), V.data() + V.size());
	}

	inline bool InRange(float low, float input, float high)
	{
		return ((input - high)*(input - low) <= 0);
	}

	inline bool InRangeExcluded(float low, float input, float high)
	{
		return ((input - high)*(input - low) < 0);
	}

	/*!
	 * @brief - Clamps a number between the minimum and max value
	 * 
	 * @param v - the value
	 * @param lo - the lowest value possible
	 * @param hi - the highest value possible
	 * @param comp - the custom function that will do the comparing
	 */
	template<class T, class Compare>
	constexpr const T& Clamp(const T& v, const T& lo, const T& hi, Compare comp)
	{
		return assert(!comp(hi, lo)),
			comp(v, lo) ? lo : comp(hi, v) ? hi : v;
	}

	/*!
	 * @brief - Clamps a number between the minimum and max value
	 *
	 * @param v - the value
	 * @param lo - the lowest value possible
	 * @param hi - the highest value possible
	 */
	template<class T>
	constexpr const T& Clamp(const T& v, const T& lo, const T& hi)
	{
		return Clamp(v, lo, hi, std::less<>());
	}

	/* TODO currently unused template<class T>
	T&& Forward(const T& rForwardValue)
	{
		return std::forward<T>(rForwardValue);
	}*/

	Quaternion FindLookAtRotation(Vector3 vStart, Vector3 vTarget);

	inline Vector3 TransformDirection(Quaternion rotation, Vector3 direction)
	{
		return direction * rotation;
	}

	inline Vector2 RandPointInCircle(float fRadius)
	{
		Vector2 point = ::Vector2(0.f);

		point.x = fRadius * cosf(static_cast<float>(rand() % FULL_CIRCLE_RADIANS));
		point.y = fRadius * sinf(static_cast<float>(rand() % FULL_CIRCLE_RADIANS));

		return point;
	}

	Vector3 MoveTowards(Vector3 current, Vector3 target, float maxDistanceDelta);

	template<typename ArrayType, unsigned S>
	// #define ARRAY_SIZE(array) (sizeof((array))/sizeof((array[0])))
	inline uint32_t Length(const ArrayType(&v)[S])
	{
		return S;
	}

	/**
	 * @brief check is variant has a base class.
	 * @param variantType
	 * @param rTypeCheck - base class that we should check
	 */
	bool CheckDerivedType(rttr::type variantType, const rttr::type& rTypeCheck);
		

	/**
	 * @brief - See if the property you are checking from is of type you give it.
	 * @param rProperty - the property that needs to be checked.
	 * @param rTypeCheck - the base class you are checking from
	*/
	bool CheckArrayDerivedType(rttr::property& rProperty, const rttr::type& rTypeCheck);


	/**
	 * @brief - See if the property you are checking from is of type you give it.
	 * @param rType - the type that needs to be checked.
	 * @param rTypeCheck - the base class you are checking from
	*/
	bool CheckArrayDerivedType(rttr::type rType, const rttr::type& rTypeCheck);


	/**
	 * @brief - Returns a random point within a given part of a sphere
	*/
	Vector3 SphericalRand(float fRadius, float fThetaMin, float fThetaMax, float fPhiMin, float fPhiMax);

#ifdef EDITOR
	void Tooltip(const rttr::property& prop);
#endif

}
