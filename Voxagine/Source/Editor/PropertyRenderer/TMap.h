#pragma once

/**
 * @brief TMap - Map wrapper for the rttr::variant_associative_view
 * 
 * ------------------------------------------------------------------
 * \code{.cpp}
 *  std::map<int, std::string> my_map = { { 1, "one" }, { 2, "two" }, { 3, "three" } };
 *  variant var = my_map;
 *  if (var.is_associative_container())
 *  {
 *      variant_associative_view view = var.create_associative_view();
 *      std::cout << view.get_size() << std::endl;      // prints: '3'
 *      for (const auto& item : view)
 *      {
 *          // remark that the key and value are stored inside a 'std::reference_wrapper'
 *          std::cout << "Key: " << item.first.extract_wrapped_value().to_string() << " ";
 *          std::cout << "Value: " << item.second.extract_wrapped_value().to_string() << std::endl;
 *      }
 *  }
 * \endcode
 *
 * \see variant
 */
class TMap
{
public:
	/**
	 * @brief - constructor
	 * 
	 * @param AssociateView
	 * 
	 * 
	 */
	TMap(rttr::variant_associative_view& AssociateView);

	rttr::variant_associative_view& Retrieve() { return m_AssociateView; }

	/**
	 * @brief - Update the key of the associative array
	 * 
	 * @param rArrView - the associative array that needs to be swapped,
	 * @param KeyArgument - The current key
	 * @param NewKeyArgument - The new key argument
	 */
	bool UpdateKey(rttr::variant_associative_view& rArrView, rttr::argument KeyArgument, rttr::argument NewKeyArgument);

	/**
	 * @brief - Update the key of the associative array
	 *
	 * @param rArrView - the associative array that needs to be swapped,
	 * @param KeyArgument - The current key
	 * @param NewValue - The new value argument
	 */
	bool UpdateValue(rttr::variant_associative_view& rArrView, rttr::argument KeyArgument, rttr::argument NewValue);

	/**
	 * @brief - Update the key of the associative array
	 *
	 * @param rArrView - the associative array that needs to be swapped,
	 * @param KeyArgument - The associated with the value
	 * @param Value - The value that needs to be inserted
	 */
	bool Insert(rttr::variant_associative_view& rArrView, rttr::argument KeyArgument, rttr::argument Value);

	// TODO make a standard member that holds the placeholder and triggers when that has been changed.
	std::pair<rttr::variant, rttr::variant> PlaceholderPair;
private:

	/**
	 * @param m_AssociateView - The associate array
	 */
	rttr::variant_associative_view m_AssociateView;
};
