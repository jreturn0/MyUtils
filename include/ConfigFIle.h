#pragma once
#include <memory>
#include <string>
#include <string_view>
#include <StringHash.h>
#include "BitFlags.h"
#include <unordered_map>
#include <variant>
#include <assert.h>
#include <shared_mutex>

namespace utl {
	enum class ConfigFlagBits : uint16_t {
		none = 0,
		archive = 1 << 0,   // Save to file
		readonly = 1 << 1,  // Cannot be changed at runtime
		notify = 1 << 2,    // Notify observers on change
		hidden = 1 << 3,    // Not shown in UIs
	};
	using ConfigFlags = utl::BitFlags<ConfigFlagBits>;


	class IConfigValueObserver {
	public:
		virtual ~IConfigValueObserver() = default;
		virtual void onValueChanged(std::string_view newValue) = 0;
	};

	enum class ConfigValueType : uint8_t {
		Bool,
		String,
		Int,
		Double,
		Unknown
	};
	using ConfigValue = std::variant<bool, std::string, int64_t, double>;


	std::string toString(const ConfigValueType type) {
		switch (type) {
		case ConfigValueType::Bool: return "Bool";
		case ConfigValueType::String: return "String";
		case ConfigValueType::Int: return "Int";
		case ConfigValueType::Double: return "Double";
		default: return "Unknown";

		}
	};


	//Check if T is a supported ConfigValue type
	template <typename T>
	concept ConfigValueTypeIsSupported =
		std::is_same_v<T, bool> ||
		std::is_same_v<T, std::string> ||
		std::is_same_v<T, std::string_view> ||
		std::is_same_v<T, const char*> ||
		std::is_integral_v<T> ||
		std::is_floating_point_v<T>;

	// Get ConfigValueType from T
	template<ConfigValueTypeIsSupported T>
	consteval ConfigValueType getConfigValueType() {
		if constexpr (std::is_same_v<T, bool>) return ConfigValueType::Bool;
		else if constexpr (std::is_same_v<T, std::string>) return ConfigValueType::String;
		else if constexpr (std::is_same_v<T, std::string_view>) return ConfigValueType::String;
		else if constexpr (std::is_same_v<T, const char*>) return ConfigValueType::String;
		else if constexpr (std::is_integral_v<T>) return ConfigValueType::Int;
		else if constexpr (std::is_floating_point_v<T>) return ConfigValueType::Double;
		else static_assert(sizeof(T) == 0, "Unsupported ConfigValue type");
		return ConfigValueType::Unknown;
	}

	// Get value from ConfigValue as T
	template< ConfigValueTypeIsSupported T>
	constexpr T getConfigValue(const ConfigValue& value) {
		if constexpr (std::is_same_v<T, int64_t>) return std::get<int64_t>(value);
		else if constexpr (std::is_same_v<T, bool>) return std::get<bool>(value);
		else if constexpr (std::is_same_v<T, std::string>) return std::get<std::string>(value);
		else if constexpr (std::is_same_v<T, std::string_view>) return std::get<std::string>(value);
		else if constexpr (std::is_same_v<T, const char*>) return std::get<std::string>(value);
		else if constexpr (std::is_same_v<T, double>) return std::get<double>(value);
		else static_assert(sizeof(T) == 0, "Unsupported ConfigValue type");
		return T{};
	}


	



	// Simple config file parser and writer
	class ConfigFile {
	public:
		struct ConfigValueInfo {
			size_t index{ 0 };
			ConfigValueType type{ ConfigValueType::Unknown };
			ConfigFlags flags;
			std::string name;
		};


		ConfigFile(std::string_view filename);

		void save();
		void load();



		/// <summary>
		/// Creates a configuration value with the specified name, default value, type, and flags.
		/// </summary>
		/// <param name="name">The name of the configuration value.</param>
		/// <param name="defaultValue">The default value to assign if no value is set.</param>
		/// <param name="type">The type of the configuration value. It MUST be the same as the types stored in ConfigValue</param>
		/// <param name="flags">Flags that specify additional properties for the configuration value. Defaults to ConfigFlagBits::archive.</param>
		/// <returns>The unique identifier (index) of the created configuration value.</returns>
		size_t createValue(std::string_view name, const ConfigValue& defaultValue, const ConfigValueType& type, ConfigFlags flags = ConfigFlagBits::archive);


		/// <summary>
		///  Creates a configuration value with the specified name, default value, and flags.
		/// </summary>
		/// <param name="name">The name of the configuration value.</param>
		/// <param name="defaultValue">The default value to assign if the configuration value does not exist.</param>
		/// <param name="flags">Flags that control the behavior of the configuration value. Defaults to ConfigFlagBits::archive.</param>
		/// <returns>The unique identifier (index) of the created configuration value.</returns>
		size_t createValue(std::string_view name, const ConfigValue& defaultValue, ConfigFlags flags = ConfigFlagBits::archive);


		bool hasValue(StringHash hash) const noexcept {
			std::shared_lock lock(m_mutex);
			return m_valueInfoMap.contains(hash);
		}








		//--- Templated methods ---//




		template<ConfigValueTypeIsSupported T>
		size_t createValue(std::string_view name, const T& defaultValue, ConfigFlags flags = ConfigFlagBits::archive) {
			std::unique_lock lock(m_mutex);
			m_values.emplace_back(defaultValue);
			size_t index = m_values.size();
			m_valueInfoMap[StringHash(name)] = { index, getConfigValueType<T>(), flags, std::string(name) };
			return index;
		}


		// Various getters 
		template<ConfigValueTypeIsSupported T>
		bool tryGetValue(StringHash hash, T& out) const {
			std::shared_lock lock(m_mutex);
			auto it = m_valueInfoMap.find(hash);
			if (it == m_valueInfoMap.end()) return false;
			const size_t idx = it->second.index;
			if (idx >= m_values.size()) return false; // corrupted index
			// Type check
			constexpr ConfigValueType expected = getConfigValueType<T>();
			if (it->second.type != expected) return false;

			out = getConfigValue<T>(m_values[idx]);
			return true;
		}




		// Set value by name hash. Returns false if not found or type mismatch
		template<ConfigValueTypeIsSupported T>
		bool setValue(StringHash name, const T& value) {
			std::unique_lock lock(m_mutex);
			if (auto it = m_valueInfoMap.find(name); it != m_valueInfoMap.end()) {
				if (it->second.flags.has(ConfigFlagBits::readonly)) {
					return false; // readonly
				}
				const size_t idx = it->second.index;
				if (idx >= m_values.size()) return false; // corrupted index
				// Type check
				constexpr ConfigValueType expected = getConfigValueType<T>();
				if (it->second.type != expected) return false;
				m_values[idx] = value;
				return true;
			}
			return false;
		}

		// Get pointer to value, or nullptr if not found or type mismatch
		template<ConfigValueTypeIsSupported T>
		const T* getValuePtr(StringHash hash) {
			std::shared_lock lock(m_mutex);
			auto it = m_valueInfoMap.find(hash);
			if (it == m_valueInfoMap.end()) return nullptr;
			const size_t idx = it->second.index;
			if (idx >= m_values.size()) return nullptr; // corrupted index
			// Type check
			constexpr ConfigValueType expected = getConfigValueType<T>();
			if (it->second.type != expected) return nullptr;
			return &getConfigValue<T>(m_values[idx]);
		}

		// Get value by index, returns false if index out of range
		template<ConfigValueTypeIsSupported T>
		bool tryGetValueByIndex(size_t index, T& out) const {
			std::shared_lock lock(m_mutex);
			if (index > m_values.size()) return false;
			out = getConfigValue<T>(m_values[index]);
			return true;
		}

		template<ConfigValueTypeIsSupported T>
		T getValue(StringHash hash) const {
			T v{};
			bool ok = tryGetValue<T>(hash, v);
			assert(ok && "ConfigFile::getValue - missing key or type mismatch");
			return ok ? v : T{};
		}




	private:
		inline void logError(const std::string& msg) const {
#ifdef CONFIG_FILE_ERROR_LOGGING
			std::cerr << "ConfigFile Error: " << msg << "\n";
#endif
		}

		std::string m_filename;
		//Hashmap of section+key to valueConfigValueInfo
		std::unordered_map<uint64_t, ConfigValueInfo> m_valueInfoMap{};
		std::vector<ConfigValue> m_values{};
		mutable std::shared_mutex m_mutex{};
	};



} // namespace utl