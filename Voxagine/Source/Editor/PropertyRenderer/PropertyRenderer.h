#pragma once

#include "External/rttr/type"

#include <unordered_map>
#include <functional>
#include <string>

class Editor;
class BaseCommand;

class PropertyRenderer
{
public:
	PropertyRenderer();
	~PropertyRenderer();

	void Initialize(Editor* pEditor);

	void Render(rttr::instance& rObject, rttr::property& rProperty, bool* bIsElementAndChanged = nullptr);
	void Render(rttr::instance& rObject, rttr::property& rProperty, std::string Label, bool* bIsElementAndChanged = nullptr);

private:
	void InitializePropertyLookUp();
	void InitializeResourcePropertyLookUp();

	void RenderProperty(rttr::instance& rInstance, rttr::property& rProperty, std::string& Label, bool* bIsElementAndChanged = nullptr);
	void RenderEnumProperty(rttr::instance& rInstance, rttr::property& rProperty, std::string& Label, bool* bIsElementAndChanged = nullptr);
	void RenderArrayProperty(rttr::instance& rInstance, rttr::property& rProperty, std::string& Label);
	void RenderMapProperty(rttr::instance& rInstance, rttr::property& rProperty, std::string& Label);

	void RenderResource(const std::string& ResourceExtension, rttr::instance& rInstance, rttr::property& rProperty, std::string Label, bool* bIsElementAndChanged = nullptr);
	void RenderResource(const std::string& ResourceExtension, rttr::instance& rInstance, rttr::property& rProperty, std::string Label);
	void RenderArrayResource(const std::string& ResourceExtension, rttr::instance& rInstance, rttr::property& rProperty, std::string& Label);

	template <typename T>
	void CreatePropertyLookUp(std::function<void(rttr::instance& rObject, rttr::property & rProperty, std::string& rLabel, bool* bIsElementAndChanged)> Function);
	template <typename T>
	void CreateArrayPropertyLookUp(std::function<void(rttr::instance& rInstance, rttr::property& rProperty, rttr::variant_sequential_view& rArrView, unsigned int uiIndex, const std::string& rLabel, bool* bIsElementAndChanged)> Function);
	template<typename K>
	void CreateKeyMapPropertyLookUp(std::function<std::pair<rttr::variant, bool>(rttr::instance&, rttr::property&, rttr::variant_associative_view&, rttr::variant&, int32_t, const std::string&)> Function);
	template<typename V>
	void CreateValueMapPropertyLookUp(std::function<std::pair<rttr::variant, bool>(rttr::instance&, rttr::property&, rttr::variant_associative_view&, rttr::variant&, rttr::variant&, int32_t, const std::string&)> Function);
	void CreateDefaultResourcePropertyLookUp(const std::string& ResourceExtension);

	void CreateResourcePropertyLookUp(const std::string& ResourceExtension, std::function<void(rttr::instance& rObject, rttr::property & rProperty, std::string& rLabel, bool* bIsElementAndChanged)> Function);

	void CreateResourcePropertyLookUp(const std::string& ResourceExtension, std::function<void(rttr::instance& rObject, rttr::property & rProperty, std::string& rLabel)> Function);
	void CreateArrayResourcePropertyLookUp(const std::string& FileExtension, std::function<bool(rttr::instance& rInstance, rttr::property& rProperty, rttr::variant_sequential_view& rArrView, unsigned int uiIndex, const std::string& rLabel)> Function);

	std::string GetResourceFileName(const std::string& FilePath) const;

	void FirePropertyCommand(rttr::instance& targetInstance, rttr::property& targetProperty, rttr::variant& redoValue) const;

	Editor* m_pEditor = nullptr;
	std::string m_PopUpSearchString = "";
	std::function<void(rttr::instance& rObject, rttr::property & rProperty, std::string& rLabel, bool* bIsElementAndChanged)> m_NotSupportedProperty;

	std::unordered_map<rttr::type, std::function<void(rttr::instance& rObject, rttr::property & rProperty, std::string& rLabel, bool* bIsElementAndChanged)>> m_PropertyLookUp;
	std::unordered_map<std::string, std::function<void(rttr::instance& rObject, rttr::property & rProperty, std::string& rLabel, bool* bIsElementAndChanged)>> m_ResourcePropertyLookUp;

	std::unordered_map<rttr::type, std::function<void(rttr::instance& rInstance, rttr::property& rProperty, rttr::variant_sequential_view& rArrView, unsigned int, const std::string& rLabel, bool* bIsElementAndChanged)>> m_ArrayPropertyLookUp;
	std::unordered_map<std::string, std::function<bool(rttr::instance& rInstance, rttr::property& rProperty, rttr::variant_sequential_view& rArrView, unsigned int, const std::string& rLabel)>> m_ArrayResourcePropertyLookUp;

	// KeyMapValue
	std::unordered_map <rttr::type, std::function<std::pair<rttr::variant, bool>(rttr::instance&, rttr::property&, rttr::variant_associative_view&, rttr::variant&, int32_t, const std::string&) >> m_KeyMapPropertyLookUp;
	// MapValue
	std::unordered_map <rttr::type, std::function<std::pair<rttr::variant, bool>(rttr::instance&, rttr::property&, rttr::variant_associative_view&, rttr::variant&, rttr::variant&, int32_t, const std::string&) >> m_ValueMapPropertyLookUp;

	std::map <std::string, std::pair<rttr::variant, rttr::variant>> m_PlaceholderMapValues = {};
};

template<typename T>
inline void PropertyRenderer::CreatePropertyLookUp(std::function<void(rttr::instance& rObject, rttr::property&rProperty, std::string& rLabel, bool*)> Function)
{
	m_PropertyLookUp[rttr::type::get<T>()] = Function;
}

template<typename T>
void PropertyRenderer::CreateArrayPropertyLookUp(std::function<void(rttr::instance&, rttr::property&, rttr::variant_sequential_view&, unsigned int, const std::string&, bool*)> Function)
{
	m_ArrayPropertyLookUp[rttr::type::get<T>()] = Function;
}

// You could do it one function but then you would need to provide multiple same type --> int,int int,string, int,double etc.
template<typename K>
void PropertyRenderer::CreateKeyMapPropertyLookUp(std::function<std::pair<rttr::variant, bool>(rttr::instance&, rttr::property&, rttr::variant_associative_view&, rttr::variant&, int32_t, const std::string&)> Function)
{
	m_KeyMapPropertyLookUp[rttr::type::get<K>()] = Function;
}

template<typename V>
void PropertyRenderer::CreateValueMapPropertyLookUp(std::function<std::pair<rttr::variant, bool>(rttr::instance&, rttr::property&, rttr::variant_associative_view&, rttr::variant&, rttr::variant&, int32_t, const std::string&)> Function)
{
	m_ValueMapPropertyLookUp[rttr::type::get<V>()] = Function;
}