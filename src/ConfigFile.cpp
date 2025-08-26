#include "ConfigFile.h"
#include "thirdparty/ini.h"
#include <iostream>
#include <charconv>
utl::ConfigFile::ConfigFile(std::string_view filename) : m_filename(filename)
{
	// Check for .ini extension
	if (m_filename.size() < 4 || m_filename.substr(m_filename.size() - 4) != ".ini") {
		m_filename += ".ini";
	}


}

namespace {
	static std::pair<std::string, std::string> parseSectionAndKey(std::string_view name) {
		auto pos = name.find('.');
		if (pos == std::string_view::npos) {
			return { "global", std::string(name) };
		}
		else {
			return { std::string(name.substr(0, pos)), std::string(name.substr(pos + 1)) };
		}
	}

	static utl::ConfigValueType guessTypeFromString(const std::string& str) {
		// Check for bool
		if (str == "true" || str == "false" || str == "1" || str == "0") {
			return utl::ConfigValueType::Bool;
		}
		// Check for int
		int64_t intValue;
		auto [ptr, ec] = std::from_chars(str.data(), str.data() + str.size(), intValue);
		if (ec == std::errc() && ptr == str.data() + str.size()) {
			return utl::ConfigValueType::Int;
		}
		// Check for double
		double doubleValue;
		auto [ptr2, ec2] = std::from_chars(str.data(), str.data() + str.size(), doubleValue);
		if (ec2 == std::errc() && ptr2 == str.data() + str.size()) {
			return utl::ConfigValueType::Double;
		}
		// Default to string
		return utl::ConfigValueType::String;
	}


	static utl::ConfigValue guessConfigValue(const std::string& value, utl::ConfigValueType& outType) {
		// Check for bool
		if (value == "true") {
			outType = utl::ConfigValueType::Bool;
			return true;
		}
		if (value == "false") {
			outType = utl::ConfigValueType::Bool;
			return false;
		}

		// Check for int
		int intVal;
		auto [ptrInt, ecInt] = std::from_chars(value.data(), value.data() + value.size(), intVal);
		if (ecInt == std::errc() && ptrInt == value.data() + value.size()) {
			outType = utl::ConfigValueType::Int;
			return intVal;
		}

		// Check for double
		double doubleVal;
		auto [ptrDouble, ecDouble] = std::from_chars(value.data(), value.data() + value.size(), doubleVal);
		if (ecDouble == std::errc() && ptrDouble == value.data() + value.size()) {
			outType = utl::ConfigValueType::Double;
			return doubleVal;
		}

		// Fallback to string
		outType = utl::ConfigValueType::String;

		return value;
	}
}
void utl::ConfigFile::save()
{
	mINI::INIFile file(m_filename);
	mINI::INIStructure ini;
	for (const auto& [hash, info] : m_valueInfoMap) {
		if (info.flags.has(ConfigFlagBits::archive)) {
			// Save this value
			const auto& value = m_values[info.index];
			std::string strValue;
			switch (info.type) {
			case ConfigValueType::Bool:
				strValue = std::get<bool>(value) ? "true" : "false";
				break;
			case ConfigValueType::Int:
				strValue = std::to_string(std::get<int64_t>(value));
				break;
			case ConfigValueType::Double:
				strValue = std::to_string(std::get<double>(value));
				break;
			case ConfigValueType::String:
				strValue = std::get<std::string>(value);
				break;
			default:
				continue; // unknown type
			}
			auto&& [section, key] = parseSectionAndKey(info.name);
			// For simplicity, use a default section
			ini[section][key] = strValue;
		}
	}
	file.write(ini);
}


void utl::ConfigFile::load()
{
	mINI::INIFile file(m_filename);
	mINI::INIStructure ini;

	// Check if string has . or e or E 
	inline auto hasFloatMarkers = [](const std::string& s) {
		return s.find_first_of(".eE") != std::string::npos;
		};



	if (file.read(ini)) {
		std::unique_lock lock(m_mutex);

		for (auto const& it : ini) {
			auto const& section = it.first;


			for (auto const& key : it.second) {
				// We should check:
				// 1. Guess type and value from string
				// 2. See if key exists
				// 3. If it exists, check type matches guess
				// 4. If it matches, set value
				// 5. If it does not match, try to convert
				// 6. 
				// 1. If the key exists in m_valueInfoMap
				// 2. If the type matches
				// 3. If not, try to guess the type and convert
				// 4. If it fails or has no value, skip it
				// 5. If it succeeds, set the value
				// 6. If the key does not exist, create it with guessed type


				utl::ConfigValueType guessedType;
				auto guessedValue = guessConfigValue(key.second, guessedType);

				if (auto vit = m_valueInfoMap.find(StringHash(section + "." + key.first)); vit != m_valueInfoMap.end()) {
					auto& info = vit->second;

					if (info.index >= m_values.size()) {
						logError("ConfigFile::load - invalid index for key " + section + "." + key.first);
						continue;
					}; // corrupted index

					auto& value = m_values[info.index];

					if (info.type == guessedType) {
						// Types match, just set the value
						value = guessedValue;
						continue;
					}
					logError("ConfigFile::load - type mismatch for key " + section + "." + key.first + ", expected " + toString(info.type) + ", got " + toString(guessedType) + ", attempting conversion");
				}
				// Key not found
				createValue(section + "." + key.first, guessedValue, ConfigFlagBits::archive);


			}
		}


	}


}

size_t utl::ConfigFile::createValue(std::string_view name, const ConfigValue& defaultValue, const ConfigValueType& type, ConfigFlags flags)
{
	std::unique_lock lock(m_mutex);
	m_values.emplace_back(defaultValue);
	size_t index = m_values.size();
	// Determine type from variant
	m_valueInfoMap[StringHash(name)] = { index, type, flags, std::string(name) };
	return index;
}

size_t utl::ConfigFile::createValue(std::string_view name, const ConfigValue& defaultValue, ConfigFlags flags)
{
	return size_t();
}


