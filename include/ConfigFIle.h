#pragma once
#include "BitFlags.h"
#include <assert.h>
#include <memory>
#include <shared_mutex>
#include <string>
#include <string_view>
#include <StringHash.h>
#include <unordered_map>
#include <variant>
#include <concepts>

#ifdef UTL_CONFIGFILE_ERROR_LOGGING
#define LOG_ERROR(fmt, ...) \
        std::cout << std::vformat(fmt, std::make_format_args(__VA_ARGS__)) << "\n"
#else
#define LOG_ERROR(fmt, ...) ((void)0)
#endif

namespace utl {

    enum class ConfigFlagBits : uint8_t {
        none = 0,
        archive = 1 << 0,   // Save to file
        readonly = 1 << 1,  // Cannot be changed at runtime
        uninitalized = 1 << 2, // Value is not initialized (for internal use)
    };
    using ConfigFlags = utl::BitFlags<ConfigFlagBits>;


    enum class ConfigValueType : uint8_t {
        String,
        Bool,
        Int,
        Double,
    };
    using ConfigValue = std::variant<std::string, bool, int64_t, double>;

    namespace details {
        // Check if ConfigValue can be constructed from T
        template<typename T>
        concept ConfigValuable = std::constructible_from<ConfigValue, T>;

        constexpr ConfigFlags g_defaultConfigFlags{ ConfigFlags(ConfigFlagBits::archive) | ConfigFlagBits::uninitalized };
        // Get ConfigValueType from T
        template<ConfigValuable T>
        inline constexpr ConfigValueType getConfigValueType() {
            if constexpr (std::is_same_v<T, bool>) return ConfigValueType::Bool;
            else if constexpr (std::is_integral_v<T>) return ConfigValueType::Int;
            else if constexpr (std::is_floating_point_v<T>) return ConfigValueType::Double;
            else return ConfigValueType::String;

        }
        inline constexpr ConfigValueType valueTypeFromVariant(const ConfigValue& value) noexcept {
            return std::visit([](auto&& val) -> ConfigValueType {
                return getConfigValueType<std::decay_t<decltype(val)>>();
                }, value);
        }

        //Convert ConfigValue of type T to string
        template <ConfigValuable T>
        inline constexpr std::string toString(ConfigValue value) {
            return std::visit([](auto&& val) -> std::string {
                using V = std::decay_t<decltype(val)>;
                if constexpr (std::is_same_v<V, T>) {
                    if constexpr (std::is_same_v<V, bool>)
                        return val ? "true" : "false";
                    else if constexpr (std::is_same_v<V, int64_t> || std::is_same_v<V, double>)
                        return std::to_string(val);
                    else if constexpr (std::is_same_v<V, std::string>)
                        return val;
                    else
                        return ""; // fallback for unknown types
                }
                else {
                    return ""; // Type mismatch
                }
                }, value);
        }


        // Get ConfigValue as string_view
        inline constexpr std::string_view toString(const ConfigValueType type) noexcept {
            switch (type) {
            case ConfigValueType::String: return "String";
            case ConfigValueType::Bool:   return "Bool";
            case ConfigValueType::Int:    return "Int";
            case ConfigValueType::Double: return "Double";
            default:                      return "Unknown";
            }
        }


        inline std::string toString(const ConfigValue& value) noexcept {
            return std::visit([](auto&& val) -> std::string {
                using T = std::decay_t<decltype(val)>;
                if constexpr (std::is_same_v<T, bool>)
                    return val ? "true" : "false";
                else if constexpr (std::is_same_v<T, int64_t> || std::is_same_v<T, double>)
                    return std::to_string(val);
                else if constexpr (std::is_same_v<T, std::string>)
                    return val;
                else
                    return ""; // fallback for unknown types
                }, value);
        }
    }


    /// <summary>
    /// Thread-safe configuration file handler. Based on INI file format. 
    /// </summary>
    class ConfigFile {
    public:
        struct ConfigValueInfo {
            size_t index{ 0 };
            ConfigValueType type{ ConfigValueType::String };
            ConfigFlags flags{ ConfigFlagBits::none };
            std::string name{ "" };
        };
        struct ConfigValueStorage {
            ConfigValue current;
            ConfigValue initial;
            ConfigValueInfo* info{ nullptr };
        };

        ConfigFile(std::string_view filename);

        void save();
        void load();

        bool hasValue(StringHash hash) const noexcept;

        size_t createValue(std::string_view name, const ConfigValue& defaultValue, ConfigFlags flags = details::g_defaultConfigFlags);

        bool getValue(StringHash hash, ConfigValue& out) const;
        bool getValue(size_t index, ConfigValue& out) const;

        


        bool setValue(StringHash name, const ConfigValue& value);
        bool setValue(size_t index, const ConfigValue& value);










#ifndef UTL_CONFIGFILE_DISABLE_TEMPLATES
        //--------------- Templated methods ---------------//
        // These are here for convenience and type safety 
        // but not strictly necessary since you can use the 
        // non-templated versions with ConfigValue directly.
        // You can disable them with #define 
        // UTL_CONFIGFILE_DISABLE_TEMPLATES
        //-------------------------------------------------//



        template<details::ConfigValuable T>
        size_t createValue(std::string_view name, const T& defaultValue, ConfigFlags flags = details::g_defaultConfigFlags) {
            std::unique_lock lock(m_mutex);
            if (auto it = m_valueInfoMap.find(StringHash(name)); it != m_valueInfoMap.end()) {
                if (it->second.flags.has(ConfigFlagBits::uninitalized)) {
                    it->second.flags = flags.without(ConfigFlagBits::uninitalized);
                    m_values[it->second.index].initial = defaultValue;
                }
                return it->second.index;
            }

            m_values.emplace_back(defaultValue, defaultValue, nullptr);
            size_t index = m_values.size() - 1;
            m_valueInfoMap[StringHash(name)] = { index, details::getConfigValueType<T>(), flags, std::string(name) };
            m_values[index].info = &m_valueInfoMap[StringHash(name)];
            return index;
        }

        // Various getters 

        // Get value by name hash, returns false if not found or type mismatch
        template<details::ConfigValuable T>
        bool getValue(StringHash hash, T& out) const {
            std::shared_lock lock(m_mutex);
            auto it = m_valueInfoMap.find(hash);
            if (it == m_valueInfoMap.end()) return false;
            const size_t index = it->second.index;
            if (index >= m_values.size()) return false; // corrupted index
            // Type check
            constexpr ConfigValueType expected = details::getConfigValueType<T>();
            if (it->second.type != expected) return false;

            auto val = std::get_if<T>(&m_values[index].current);
            if (!val) return false;
            out = *val;
            return true;
        }



        // Get value by index, returns false if index out of range or type mismatch
        template<details::ConfigValuable T>
        bool getValue(size_t index, T& out) const {
            std::shared_lock lock(m_mutex);
            if (index > m_values.size()) return false;
            auto&& [current, initial, info] = m_values[index];
            if (info->type != details::getConfigValueType<T>()) return false;
            out = std::get<T>(current);
            return true;
        }


        // Get initial value by name hash, returns false if not found or type mismatch
        template<details::ConfigValuable T>
        bool getInitialValue(StringHash hash, T& out) const {
            std::shared_lock lock(m_mutex);
            auto it = m_valueInfoMap.find(hash);
            if (it == m_valueInfoMap.end()) return false;
            const size_t index = it->second.index;
            if (index >= m_values.size()) return false; // corrupted index
            // Type check
            constexpr ConfigValueType expected = details::getConfigValueType<T>();
            if (it->second.type != expected) return false;

            auto val = std::get_if<T>(&m_values[index].initial);
            if (!val) return false;
            out = *val;
            return true;
        }

        template<details::ConfigValuable T>
        bool getInitialValue(size_t index, T& out) const {
            std::shared_lock lock(m_mutex);
            if (index > m_values.size()) return false;
            auto&& [current,initial, info] = m_values[index];
            if (info->type != details::getConfigValueType<T>()) return false;
            out = std::get<T>(initial);
            return true;
        }

        template<details::ConfigValuable T>
        T getValueCopyOrDefault(StringHash hash, const T& defaultValue) const {
            T out{};
            if (getValue(hash, out)) {
                return out;
            }
            return defaultValue;
        }

        // Get pointer to value, or nullptr if not found or type mismatch
        template<details::ConfigValuable T>
        const T* getValuePtr(StringHash hash) {
            std::shared_lock lock(m_mutex);
            auto it = m_valueInfoMap.find(hash);
            if (it == m_valueInfoMap.end()) return nullptr;
            const size_t index = it->second.index;
            if (index >= m_values.size()) return nullptr; // corrupted index
            // Type check
            constexpr ConfigValueType expected = details::getConfigValueType<T>();
            if (it->second.type != expected) return nullptr;
            return std::get_if<T>(&m_values[index].current);
        }
        // Get pointer to value, or nullptr if not found or type mismatch
        template<details::ConfigValuable T>
        const T* getValuePtr(size_t index) {
            std::shared_lock lock(m_mutex);
            if (index >= m_values.size()) return nullptr; // corrupted index
            auto&& [current, initial, info] = m_values[index];
            // Type check
            constexpr ConfigValueType expected = details::getConfigValueType<T>();
            if (info->type != expected) return nullptr;
            return std::get_if<T>(&current);
        }


        // Set value by name hash. Returns false if not found or type mismatch
        template<details::ConfigValuable T>
        bool setValue(StringHash name, const T& value) {
            std::unique_lock lock(m_mutex);
            if (auto it = m_valueInfoMap.find(name); it != m_valueInfoMap.end()) {
                if (it->second.flags.has(ConfigFlagBits::readonly)) {
                    return false; // readonly
                }
                const size_t index = it->second.index;
                if (index >= m_values.size()) return false; // corrupted index
                // Type check
                constexpr ConfigValueType expected = details::getConfigValueType<T>();
                if (it->second.type != expected) return false;
                m_values[index].current = value;
                return true;
            }
            return false;
        }

        // Set value by index. Returns false if index out of range or type mismatch
        template<details::ConfigValuable T>
        bool setValue(size_t index, const T& value) {
            std::unique_lock lock(m_mutex);
            if (index >= m_values.size()) return false;
            auto&& [current, initial, info] = m_values[index];
            // Check readonly flag
            if (info->flags.has(ConfigFlagBits::readonly)) {
                return false; // readonly
            }
            // Type check
            constexpr ConfigValueType expected = details::getConfigValueType<T>();
            if (info->type != expected) return false;
            current = value;
            return true;
        }




#endif

        std::vector< ConfigValueStorage>& getAllValues() noexcept { return m_values; }

    private:





        std::string m_filename;
        std::unordered_map<uint64_t, ConfigValueInfo> m_valueInfoMap{};
        std::vector<ConfigValueStorage> m_values{};
        mutable std::shared_mutex m_mutex{};
    };



}; // namespace utl