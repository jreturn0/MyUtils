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

    template <typename T>
    requires std::is_arithmetic_v<T>
    static bool tryStringToNumber(const std::string& str, T& out) {
        auto [ptr, ec] = std::from_chars(str.data(), str.data() + str.size(), out);
        return (ec == std::errc() && ptr == str.data() + str.size());
    }



    static utl::ConfigValueType guessTypeFromString(const std::string& str) {
        // Check for bool
        if (str == "true" || str == "false") {
            return utl::ConfigValueType::Bool;
        }
        // Check for int
        int64_t intValue;
        if(tryStringToNumber(str,intValue)) {
            return utl::ConfigValueType::Int;
        }
        // Check for double
        double doubleValue;
        if (tryStringToNumber(str, doubleValue)) {
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
        int64_t intValue;
        if (tryStringToNumber(value, intValue)) {
            outType= utl::ConfigValueType::Int;
            return intValue;
        }
        // Check for double
        double doubleValue;
        if (tryStringToNumber(value, doubleValue)) {
            outType= utl::ConfigValueType::Double;
            return doubleValue;
        }

        // Fallback to string
        outType = utl::ConfigValueType::String;

        return value;
    }



    // Get inner configValue configValueType






}
void utl::ConfigFile::save()
{
    mINI::INIFile file(m_filename);
    mINI::INIStructure ini;
    for (auto&& [value,inital, info] : m_values) {
        if (info->flags.has(ConfigFlagBits::archive)) {        
            auto&& [section, key] = parseSectionAndKey(info->name);
            ini[section][key] = utl::details::toString(value);
        }
    }
    file.write(ini);
}


void utl::ConfigFile::load()
{
    auto loadCreateValue = [this](std::string_view name, const ConfigValue& defaultValue) {
        m_values.emplace_back(defaultValue, defaultValue, nullptr);
        size_t index = m_values.size() - 1;
        m_valueInfoMap[StringHash(name)] = { index, details::valueTypeFromVariant(defaultValue), details::g_defaultConfigFlags, std::string(name) };
        auto& storage = m_values[index];
        storage.info= &m_valueInfoMap[StringHash(name)];
        };

    mINI::INIFile file(m_filename);
    mINI::INIStructure ini;
    if (file.read(ini)) {
        std::unique_lock lock(m_mutex);
        for (auto const& it : ini) {
            auto const& section = it.first;
            for (auto&& [key,value] : it.second) {
                utl::ConfigValueType guessedType;
                auto guessedValue = guessConfigValue(value, guessedType);
                // Check if key exists
                auto sectionKey = section + "." + key;
                if (auto vit = m_valueInfoMap.find(StringHash(sectionKey)); vit != m_valueInfoMap.end()) {
                    auto& info = vit->second;

                    if (info.index >= m_values.size()) {
                        LOG_ERROR("ConfigFile::load - invalid index for key {}.{}", section,".",key);
                        continue;
                    };
                    if (info.type != guessedType) {
                        LOG_ERROR("ConfigFile::load - type mismatch for key {}.{}, expected {}, got {}", section, key,  utl::details::toString(info.type), utl::details::toString(guessedType));
                        continue;
                    }
                    auto& storage = m_values[info.index];
                    storage.current = std::move(guessedValue);
                    continue;
                }
                // Else create new value
                loadCreateValue(sectionKey, guessedValue);
            }
        }
    }
}

bool utl::ConfigFile::hasValue(StringHash hash) const noexcept
{
    std::shared_lock lock(m_mutex);
    return m_valueInfoMap.contains(hash);
}



size_t utl::ConfigFile::createValue(std::string_view name, const ConfigValue& defaultValue, ConfigFlags flags)
{
    std::unique_lock lock(m_mutex);
    // If already exists, return existing index
    if (auto it = m_valueInfoMap.find(StringHash(name)); it != m_valueInfoMap.end()) {
        if (it->second.flags.has(ConfigFlagBits::uninitalized)) {
            it->second.flags = flags.without(ConfigFlagBits::uninitalized);
            m_values[it->second.index].initial = defaultValue;
        }
        return it->second.index;
    }

    m_values.emplace_back(defaultValue,defaultValue,nullptr);
    size_t index = m_values.size()-1;
    m_valueInfoMap[StringHash(name)] = { index, details::valueTypeFromVariant(defaultValue), flags, std::string(name) };
    m_values[index].info = &m_valueInfoMap[StringHash(name)];
    return size_t();
}

bool utl::ConfigFile::getValue(StringHash hash, ConfigValue& out) const
{
    std::shared_lock lock(m_mutex);
    auto it = m_valueInfoMap.find(hash);
    if (it == m_valueInfoMap.end()) return false; // not found
    const size_t idx = it->second.index;
    if (idx >= m_values.size()) return false; // corrupted index
    out = m_values[idx].current;
    return true;
}

bool utl::ConfigFile::getValue(size_t index, ConfigValue& out) const
{
    std::shared_lock lock(m_mutex);
    if (index >= m_values.size()) return false;
    out = m_values[index].current;
    return true;
}



bool utl::ConfigFile::setValue(StringHash name, const ConfigValue& value)
{
    std::unique_lock lock(m_mutex);
    if (auto it = m_valueInfoMap.find(name); it != m_valueInfoMap.end()) {
        if (it->second.flags.has(ConfigFlagBits::readonly)) {
            return false; // readonly
        }
        const size_t idx = it->second.index;
        if (idx >= m_values.size()) return false; // corrupted index
        auto newType = details::valueTypeFromVariant(value);
        if (it->second.type != newType) return false;
        m_values[idx].current = value;
        return true;
    }
    return false;
    
}

bool utl::ConfigFile::setValue(size_t index, const ConfigValue& value)
{
    std::unique_lock lock(m_mutex);
    if (index >= m_values.size()) return false;
    // Check readonly flag
    for (const auto& [hash, info] : m_valueInfoMap) {
        if (info.index == index) {
            if (info.flags.has(ConfigFlagBits::readonly)) {
                return false; // readonly
            }
            // Type check
            ConfigValueType expected = details::valueTypeFromVariant(value);
            if (info.type != expected) return false;
            m_values[index].current = value;
            return true;
        }
    }
    return false; // index not found in info map
}


