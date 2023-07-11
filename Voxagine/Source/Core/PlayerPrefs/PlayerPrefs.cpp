#include "pch.h"
#include "PlayerPrefs.h"
#include "Core/Application.h"

RTTR_REGISTRATION
{

#ifdef _DEBUG
	std::cout << "Creating reflection for: PlayerPrefs" << std::endl;
	std::cout << "Creating reflection for: PlayerPrefs::Pair" << std::endl;
#endif
	rttr::registration::class_<PlayerPrefs>("PlayerPrefs")
	.constructor<>()(rttr::policy::ctor::as_object)
	.property("PlayerPreferences", &PlayerPrefs::m_PlayerPreferences);

	rttr::registration::class_<PlayerPrefs::Pair>("PlayerPrefs::Pair")
		.constructor<>()(rttr::policy::ctor::as_raw_ptr);
}

std::map<std::string, PlayerPrefs::Pair*> PlayerPrefs::m_PlayerPreferences = {};
BoolPrefAccessor PlayerPrefs::boolAccessor;
DoublePrefAccessor PlayerPrefs::DoubleAccessor;
DateTimePrefAccessor PlayerPrefs::DateTimeAccessor;

PlayerPrefs* PlayerPrefs::s_pInstance = nullptr;
bool PlayerPrefs::bIsInitialized = false;

bool BoolPrefAccessor::Get(const std::string& prefKey, bool defaultValue)
{
	const auto prefValue = PlayerPrefs::GetInt(prefKey, static_cast<int>(defaultValue));
	return (prefValue == 1);
}

PrefAccessor<bool>& BoolPrefAccessor::Set(const std::string& prefKey, bool prefValue)
{
	PlayerPrefs::SetInt(prefKey, static_cast<int>(prefValue));
	return *this;
}

double DoublePrefAccessor::Get(const std::string& prefKey, double defaultValue)
{
	const auto prefValue = std::stod(PlayerPrefs::GetString(prefKey, std::to_string(defaultValue)));
	return prefValue;
}

PrefAccessor<double>& DoublePrefAccessor::Set(const std::string& prefKey, double prefValue)
{
	PlayerPrefs::SetString(prefKey, std::to_string(prefValue));
	return *this;
}

DateTime DateTimePrefAccessor::Get(const std::string& prefKey, DateTime defaultValue)
{
	const auto storedValue = PlayerPrefs::GetString(prefKey, std::to_string(defaultValue.GetTime()));
	return DateTime::Parse(storedValue);
}

PrefAccessor<DateTime>& DateTimePrefAccessor::Set(const std::string& prefKey, DateTime prefValue)
{
	PlayerPrefs::SetString(prefKey, prefValue.ToString());
	return *this;
}

void PlayerPrefs::DeleteAll()
{
	m_PlayerPreferences.clear();
	Save();
}

void PlayerPrefs::DeleteKey(const std::string& key)
{
	const auto found = m_PlayerPreferences.find(key);
	if(found != m_PlayerPreferences.end())
	{
		m_PlayerPreferences.erase(key);
	}
}

float PlayerPrefs::GetFloat(const std::string& key, float defaultValue)
{
	const auto found = m_PlayerPreferences.find(key);
	if (found != m_PlayerPreferences.end())
	{
		const auto floatValue = dynamic_cast<FloatPlayerPair*>(found->second);
		defaultValue = floatValue->value;
	}

	return defaultValue;
}

int PlayerPrefs::GetInt(const std::string& key, int defaultValue)
{
	const auto found = m_PlayerPreferences.find(key);
	if (found != m_PlayerPreferences.end())
	{
		const auto intValue = dynamic_cast<IntPlayerPair*>(found->second);
		defaultValue = intValue->value;
	}

	return defaultValue;
}

std::string PlayerPrefs::GetString(const std::string& key, std::string defaultValue)
{
	const auto found = m_PlayerPreferences.find(key);
	if (found != m_PlayerPreferences.end())
	{
		const auto test = dynamic_cast<StringPlayerPair*>(found->second);
		defaultValue = test->value;
	}

	return defaultValue;
}

bool PlayerPrefs::HasKey(const std::string& key)
{
	return (m_PlayerPreferences.find(key) != m_PlayerPreferences.end());
}

void PlayerPrefs::Save()
{
	assert(bIsInitialized && "You forget to initialize the player preferences");
	GetInstance().SaveSettings();
}

void PlayerPrefs::SetFloat(const std::string& key, const float& value)
{
	if(HasKey(key))
	{
		if(const auto& pair = dynamic_cast<FloatPlayerPair*>(m_PlayerPreferences[key])) 
		{
			pair->value = value;
			m_PlayerPreferences[key] = pair;
		}
	}
	else 
	{
		auto pair = new FloatPlayerPair;
		pair->value = value;
		m_PlayerPreferences.insert({ key, pair });
	}
}

void PlayerPrefs::SetInt(const std::string& key, const int& value)
{
	if (HasKey(key))
	{
		if (const auto& pair = dynamic_cast<IntPlayerPair*>(m_PlayerPreferences[key]))
		{
			pair->value = value;
			m_PlayerPreferences[key] = pair;
		}
	}
	else
	{
		auto pair = new IntPlayerPair;
		pair->value = value;
		m_PlayerPreferences.insert({ key, pair });
	}
}

void PlayerPrefs::SetString(const std::string& key, const std::string& value)
{
	if (HasKey(key))
	{
		if (const auto& pair = dynamic_cast<StringPlayerPair*>(m_PlayerPreferences[key]))
		{
			pair->value = value;
			m_PlayerPreferences[key] = pair;
		}
	}
	else
	{
		auto pair = new StringPlayerPair;
		pair->value = value;
		m_PlayerPreferences.insert({ key, pair });
	}
}

PlayerPrefs::PlayerPrefs()
{
	if (s_pInstance)
		assert(false && "We already have an instance of this object");

	s_pInstance = this;
}

void PlayerPrefs::Initialize(JsonSerializer* pSerializer, const std::string& filePath)
{
	bIsInitialized = true;
	BaseSettings::Initialize(pSerializer, filePath);
}


PlayerPrefs& PlayerPrefs::GetInstance()
{
	return *s_pInstance;
}