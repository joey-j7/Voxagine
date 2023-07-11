#pragma once

#include <External/rttr/type>
#include <External/rttr/registration>
#include <External/rttr/registration_friend> 

#include "Editor/Configuration/BaseSettings.h"

#include "Core/Utils/DateTime.h"


class Application;

template<typename Type>
struct PrefAccessor {};

/** 
 * @brief PowerPrefs are extra implementation
 * settings to have inside the normal player prefs.
 * With this you can expand the way of saving.
 * Keep in mind that you're still bound to the int, double, float, string
 *
 */
template<typename Type>
class PowerPrefsAccessor : public PrefAccessor<Type>
{
public:
	PowerPrefsAccessor() = default;
	virtual ~PowerPrefsAccessor() = default;

	/*
	 * @brief Get a type value from the PlayerPrefs.
	 * @param prefKey - The key to retrieve the value for
	 * @param defaultValue - The default value to return if the key doesn't exist. if not specified it will be the built-in default.
	 *
	 * @return Type - the typed value stored at the key prefKey or if not present then the built-in default.
	 */
	virtual Type Get(const std::string& prefKey, Type defaultValue) = 0;

	/*
	 * @brief - Set a typed value into PlayerPrefs
	 * @param prefKey - The key to set a value for.
	 * @param prefValue - The value to set.
	 */
	virtual PrefAccessor<Type>& Set(const std::string& prefKey, Type prefValue) = 0;
};

class BoolPrefAccessor : public PowerPrefsAccessor<bool>
{
public:
	virtual ~BoolPrefAccessor() = default;

	bool Get(const std::string& prefKey, bool defaultValue) override;

	PrefAccessor<bool>& Set(const std::string& prefKey, bool prefValue) override;
};

class DoublePrefAccessor : public PowerPrefsAccessor<double>
{
public:
	virtual ~DoublePrefAccessor() = default;

	/**
	 * @brief - Return the value corresponding to key in the preference file if it exists.
	 * @param prefKey - fetch with the key the double value
	 * @param defaultValue - return the default value when not found.
	 * @return returns the double value ( can be the default value ).
	 */
	double Get(const std::string& prefKey, double defaultValue) override;

	/**
	 * @brief - Sets the value of the preference identified by key.
	 * @param prefKey - this script sets the double up in the PlayerPrefs to be used elsewhere
	 * @param prefValue -
	*/
	PrefAccessor<double>& Set(const std::string& prefKey, double prefValue) override;
};

class DateTimePrefAccessor : public PowerPrefsAccessor<DateTime>
{
public:
	virtual ~DateTimePrefAccessor() = default;

	DateTime Get(const std::string& prefKey, DateTime defaultValue) override;

	PrefAccessor<DateTime>& Set(const std::string& prefKey, DateTime prefValue) override;
};

class PlayerPrefs : public BaseSettings
{
public:
	struct Pair
	{
		virtual ~Pair() = default;

		RTTR_ENABLE();
	};

protected:
	template<typename Type>
	struct PlayerPair : Pair
	{
		virtual ~PlayerPair() = default;
		Type value = Type();

		RTTR_ENABLE(Pair);
	};
public:
	using IntPlayerPair = PlayerPair<int32_t>;
	using FloatPlayerPair = PlayerPair<float>;
	using StringPlayerPair = PlayerPair<std::string>;

	PlayerPrefs();
	virtual ~PlayerPrefs() = default;

	/**
	 * @brief - Removes all keys and values from the preferences. (Be careful when using this function) 
	 */
	static void			DeleteAll();

	/**
	 * @brief - Removes key and its corresponding value from the preferences
	 * @param key - 
	 */
	static void			DeleteKey(const std::string& key);

	/**
	 * @brief - Returns the value corresponding to key in the preference file if it exists.
	 * @param key - fetch with the key the float value
	 * @param defaultValue - return the default value when not found.
	 * @return returns the float value ( can be the default value ).
	 */
	static float		GetFloat(const std::string& key, float defaultValue = 0.0f);

	/**
	 * @brief - Returns the value corresponding to key in the preference file if it exists.
	 * @param key - fetch with the key the int value
	 * @param defaultValue - return the default value when not found.
	 * @return returns the int value ( can be the default value ).
	 */
	static int32_t		GetInt(const std::string& key, int32_t defaultValue = 0);

	/**
	 * @brief - Returns the value corresponding to key in the preference file if it exists.
	 * @param key - fetch with the key the string value
	 * @param defaultValue - return the default value when not found.
	 * @return returns the string value ( can be the default value ).
	 */
	static std::string	GetString(const std::string& key, std::string defaultValue = "");

	/**
	 * @brief - Returns true if key exists in the preferences.
	 * @param key - this script checks if the key exists in the PlayerPrefs
	 */
	static bool			HasKey(const std::string& key);

	/**
	 * @brief - Writes all modified preferences to disk.
	 *
	 * @important - please use this function instead of saving it 
	 * through SaveSettings, it will not check if the PlayerPrefs is 
	 * already initialized.
	 */
	static void			Save();
	
	/**
	 * @brief - SetFloat Sets the value of the preference identified by key.
	 * @param key - this script sets the float up in the PlayerPrefs to be used elsewhere
	 * @param value
	*/
	static void			SetFloat(const std::string& key, const float& value);

	/**
	 * @brief - Sets the value of the preference identified by key.
	 * @param key - this script sets the int up in the PlayerPrefs to be used elsewhere
	 * @param value
	 */
	static void			SetInt(const std::string& key, const int32_t& value);

	/**
	 * @brief - Sets the value of the preference identified by key.
	 * @param key - 
	 * @param value
	 */
	static void			SetString(const std::string& key, const std::string& value);

	/**
	 * @brief - Grabs the powerpref item to store booleans
	 * @return PowerPrefs
	 */
	static BoolPrefAccessor& GetBoolAccessor() { return boolAccessor; }

	/**
	* @brief - Grabs the powerpref item to store doubles
	* @return PowerPrefs
	*/
	static DoublePrefAccessor& GetDoubleAccessor() { return DoubleAccessor; }

	/**
	* @brief - Grabs the powerpref item to store Datetime
	* @return PowerPrefs
	*/
	static DateTimePrefAccessor& GetDateTimeAccessor() { return DateTimeAccessor; }

	void Initialize(JsonSerializer* pSerializer, const std::string& filePath) override;

	RTTR_ENABLE(BaseSettings);
	RTTR_REGISTRATION_FRIEND;

protected:
	static BoolPrefAccessor boolAccessor;
	static DoublePrefAccessor DoubleAccessor;
	static DateTimePrefAccessor DateTimeAccessor;

	static PlayerPrefs&	GetInstance();

	static std::map<std::string, Pair*> m_PlayerPreferences;

private:
	static PlayerPrefs* s_pInstance;
	static bool bIsInitialized;
};
