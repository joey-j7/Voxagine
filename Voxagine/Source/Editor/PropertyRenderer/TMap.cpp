#include "pch.h"
#include "TMap.h"

TMap::TMap(rttr::variant_associative_view& AssociateView)
{
	m_AssociateView = AssociateView;
}

bool TMap::UpdateKey(rttr::variant_associative_view& rArrView, rttr::argument KeyArgument, rttr::argument NewKeyArgument)
{
	// Retrieve the map. we already have it stored
	const auto& itr = m_AssociateView.find(KeyArgument);
	if (itr != m_AssociateView.end())
	{
		// grab the value pair
		rttr::variant ValuePairVariant = itr.get_value();
		ValuePairVariant.convert(m_AssociateView.get_value_type());

		// Erase the key,value pair
		m_AssociateView.erase(KeyArgument);

		// Reinsert the updated Key
		return Insert(rArrView, NewKeyArgument, ValuePairVariant);
	}

	return false;
}

bool TMap::UpdateValue(rttr::variant_associative_view& rArrView, rttr::argument KeyArgument, rttr::argument NewValue)
{
	// Retrieve the map. we already have it stored
	const auto& itr = m_AssociateView.find(KeyArgument);
	if (itr != m_AssociateView.end())
	{
		// Erase the key,value pair first
		m_AssociateView.erase(KeyArgument);

		return Insert(rArrView, KeyArgument, NewValue);
	}

	return false;
}

bool TMap::Insert(rttr::variant_associative_view& rArrView, rttr::argument KeyArgument, rttr::argument Value)
{
	const auto& insert = m_AssociateView.insert(KeyArgument, Value);

	// If the insertion worked
	if (insert.second)
	{
		auto& NewValue = Retrieve();
		rArrView.swap(NewValue);
		return true;
	}

	return false;
}
