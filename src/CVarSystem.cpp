#include "CVarSystem.h"
#include <shared_mutex>
#include "thirdparty/ini.h"
#include <iostream>
#include <format>

namespace utl {

	namespace {



		using CVarValue = std::variant<bool, std::string, int64_t, double>;
		enum class CVarType : uint8_t {
			Float,
			Int,
			Bool,
			String,
			Unknown
		};

		template<typename T>
		constexpr CVarType getCVarType() {
			if constexpr (std::is_same_v<T, int64_t>) return CVarType::Int;
			else if constexpr (std::is_same_v<T, bool>) return CVarType::Bool;
			else if constexpr (std::is_same_v<T, std::string>) return CVarType::String;
			else if constexpr (std::is_same_v<T, std::string_view>) return CVarType::String;
			else if constexpr (std::is_same_v<T, double>) return CVarType::Float;
			else static_assert(sizeof(T) == 0, "Unsupported CVar type");
			return CVarType::Unknown;
		}

	}


	struct CVarParameter
	{
		size_t index{ 0 };

		CVarType type{ CVarType::Unknown };
		CVarFlags flags;

		std::string name;
		std::string description;
	};
	namespace {


		static constexpr size_t g_maxCVars = 2048;


		struct CVarStorage
		{
			CVarValue initial{};
			CVarValue current{};
			CVarParameter* info{ nullptr };
		};

		template <size_t N>
		struct CVarArray
		{
			std::array<CVarStorage, N> data{};
			size_t lastCVar{ 0 };

			CVarArray() = default;




			CVarStorage getStorage(size_t index)noexcept { return index >= data.size() ? CVarStorage{} : data[index]; }
			CVarStorage* getStoragePtr(size_t index)noexcept { return  index >= data.size() ? nullptr : &data[index]; }

			CVarValue getCurrent(size_t index) noexcept { return  index >= data.size() ? CVarValue{} : data[index].current; }

			CVarValue* getCurrentPtr(size_t index) noexcept { return  index >= data.size() ? nullptr : &data[index].current; }



			auto begin() { return data.begin(); }
			auto end() { return data.begin() + lastCVar; }


			bool setCurrent(size_t index, const CVarValue& value)
			{
				if (index >= data.size()) return false;
				data[index].current = value;
				return true;
			}

			size_t add(const CVarValue& initial, const CVarValue& current, CVarParameter* info)
			{
				if (lastCVar >= data.size()) {
					throw std::runtime_error("Too many CVars");
				}
				auto& prop = data[lastCVar];
				prop.initial = initial;
				prop.current = current;
				prop.info = info;
				info->index = lastCVar;
				++lastCVar;
				return lastCVar - 1ll;
			}

			size_t add(const CVarValue& value, CVarParameter* info)
			{
				return add(value, value, info);
			}


		};
	} // namespace


	class CVarSystemImpl : public CVarSystem
	{

	public:
		// Get CVarType from T


		// Generic getter for CVarParameter by nameHash
		CVarParameter* getCVar(StringHash nameHash) noexcept {
			std::shared_lock lock(m_sharedMutex);
			const auto it = m_parameterMap.find(nameHash);
			if (it == m_parameterMap.end()) {
				return nullptr;
			}
			return &it->second;
		};

		CVarParameter* getCVar(uint64_t nameHash) noexcept {
			std::shared_lock lock(m_sharedMutex);
			const auto it = m_parameterMap.find(nameHash);
			if (it == m_parameterMap.end()) {
				return nullptr;
			}
			return &it->second;
		};

		// Generic getter for CVar current value by nameHash and type T
		template<typename T>
		T* getCVarCurrent(uint64_t nameHash) noexcept {
			std::shared_lock lock(m_sharedMutex);
			auto* info = getCVar(nameHash);
			if (!info) return nullptr;
			auto* val = m_storage.getCurrentPtr(info->index);
			if (!val) return nullptr;
			return std::get_if<T>(val);
		}

		template <typename T>
		bool setCVarCurrent(uint64_t nameHash, const T& value) noexcept {
			std::unique_lock lock(m_sharedMutex);
			auto* param = getCVar(nameHash);
			if (!param) return false;
			if (param->type != getCVarType<T>()) return false;
			m_storage.setCurrent(param->index, CVarValue(value));
			return true;
		}


		template<typename T>
		CVarParameter* createCVar(std::string_view name, CVarValue defaultValue, T currentValue, CVarFlags flags, std::string_view optDescription) noexcept {
			std::unique_lock lock(m_sharedMutex);
			CVarParameter* param = initCVar(name, getCVarType<T>(), flags, optDescription);
			if (!param) return nullptr;
			m_storage.add(CVarValue{ defaultValue }, CVarValue{ currentValue }, param);
			return param;

		}

		auto begin() { return m_parameterMap.begin(); }
		auto end() { return m_parameterMap.end(); }
		CVarArray<g_maxCVars>& getStorage() { return m_storage; }


		static CVarSystemImpl* getInstance()
		{
			return dynamic_cast<CVarSystemImpl*>(CVarSystem::getInstance());
		}



		double getFloatCVar(StringHash hash) noexcept override final
		{
			return *getCVarCurrent<double>(hash);
		}

		int64_t getIntCVar(StringHash hash) noexcept override final
		{
			return *getCVarCurrent<int64_t>(hash);
		}

		bool getBoolCVar(StringHash hash) noexcept override final
		{
			return *getCVarCurrent<bool>(hash);
		}
		std::string_view getStringCVar(StringHash hash) noexcept override final
		{
			return *getCVarCurrent<std::string>(hash);
		}


		// Inherited via CVarSystem
		CVarParameter* getCVarParameter(StringHash name) noexcept override final
		{
			return getCVar(name);
		}

		CVarParameter* createFloatCVar(std::string_view name, double defaultValue, CVarFlags flags, std::string_view description) noexcept override final
		{
			return createCVar(name, defaultValue, defaultValue, flags, description);
		}

		CVarParameter* createIntCVar(std::string_view name, int64_t defaultValue, CVarFlags flags, std::string_view description) noexcept override final
		{
			return createCVar(name, defaultValue, defaultValue, flags, description);
		}

		CVarParameter* createBoolCVar(std::string_view name, bool defaultValue, CVarFlags flags, std::string_view description) noexcept override final
		{
			return createCVar(name, defaultValue, defaultValue, flags, description);
		}

		CVarParameter* createStringCVar(std::string_view name, std::string_view defaultValue, CVarFlags flags, std::string_view description) noexcept override final
		{
			return createCVar(name, std::string(defaultValue), std::string(defaultValue), flags, description);
		}

		void setFloatCVar(StringHash hash, double values) noexcept override final
		{
			setCVarCurrent(hash, values);
		}

		void setIntCVar(StringHash hash, int64_t values) noexcept override final
		{
			setCVarCurrent(hash, values);
		}

		void setBoolCVar(StringHash hash, bool values) noexcept override final
		{
			setCVarCurrent(hash, values);
		}

		void setStringCVar(StringHash hash, std::string_view values) noexcept override final
		{
			setCVarCurrent(hash, std::string(values));
		}



		constexpr std::string_view toString(CVarType type) {
			switch (type) {
			case CVarType::Float: return "Float";
			case CVarType::Int: return "Int";
			case CVarType::Bool: return "Bool";
			case CVarType::String: return "String";
			default: return "Unknown";
			}
		}

		void debugPrintCVars() override final {
			std::shared_lock lock(m_sharedMutex);
			for (const auto& [hash, param] : m_parameterMap)
			{
				auto* current = m_storage.getCurrentPtr(param.index);
				if (!current) continue;
				std::visit([&](auto&& arg) {

					std::cout << std::format("name: {}\nvalue: {}\ntype: {}\nflags: {}\nindex:{}\ndescription: {}\n", param.name, arg, toString(param.type), static_cast<uint16_t>(param.flags), param.index, param.description);
					}, *current);
			}
		}


	private:
		std::shared_mutex m_sharedMutex;
		std::unordered_map<uint64_t, CVarParameter> m_parameterMap;
		CVarArray<g_maxCVars> m_storage;


		CVarParameter* initCVar(const std::string_view name, const  CVarType type, const CVarFlags flags, const std::string_view optDescription)
		{
			//std::unique_lock lock(mutex);
			auto hash = StringHash(name);
			auto& info = m_parameterMap[hash];
			info.name = name;
			info.description = optDescription;
			info.type = type;
			info.flags = flags;
			return &info;
		}
	};
	CVarSystem* CVarSystem::getInstance()
	{
		static CVarSystemImpl cvarSys{};
		return &cvarSys;
	}
	template<typename T>
	T getCVarCurrentByIndex(uint64_t index) {

		auto cur = CVarSystemImpl::getInstance()->getStorage().getCurrent(index);

		if (auto val = std::get_if<T>(&cur)) {
			return *val;
		}
		return {};
	}
	template<typename T>
	T* ptrGetCVarCurrentByIndex(uint64_t index) {
		CVarValue* cur = CVarSystemImpl::getInstance()->getStorage().getCurrentPtr(index);

		if (!cur) return nullptr;

		if (auto val = std::get_if<T>(cur)) {
			return val; // already a T*
		}
		return nullptr;
	}



	template<typename T>
	void setCVarCurrentByIndex(uint64_t  index, const T& data) {
		CVarSystemImpl::getInstance()->getStorage().setCurrent(index, CVarValue(data));
	}


	AutoCVar_Float::AutoCVar_Float(std::string_view name, double defaultValue, CVarFlags flags, std::string_view description)
	{
		index = CVarSystemImpl::getInstance()->createFloatCVar(name, defaultValue, flags, description)->index;
	}

	double AutoCVar_Float::get() const
	{
		return getCVarCurrentByIndex<double>(index);
	}

	double* AutoCVar_Float::getPtr() const
	{
		return ptrGetCVarCurrentByIndex<double>(index);
	}

	float AutoCVar_Float::getFloat()
	{
		return static_cast<float>(getCVarCurrentByIndex<double>(index));
	}

	float* AutoCVar_Float::getFloatPtr()
	{
		return reinterpret_cast<float*>(ptrGetCVarCurrentByIndex<double>(index));
	}

	void AutoCVar_Float::set(double val) const
	{
		setCVarCurrentByIndex<double>(index, val);
	}

	AutoCVar_Int::AutoCVar_Int(std::string_view name, int64_t defaultValue, CVarFlags flags, std::string_view description)
	{
		index = CVarSystemImpl::getInstance()->createIntCVar(name, defaultValue, flags, description)->index;
	}

	int64_t AutoCVar_Int::get() const
	{
		return getCVarCurrentByIndex<int64_t>(index);
	}
	int64_t* AutoCVar_Int::getPtr() const
	{
		return ptrGetCVarCurrentByIndex<int64_t>(index);
	}
	void AutoCVar_Int::set(int64_t val) const
	{
		setCVarCurrentByIndex<int64_t>(index, val);
	}

	AutoCVar_Bool::AutoCVar_Bool(std::string_view name, bool defaultValue, CVarFlags flags, std::string_view description)
	{
		index = CVarSystemImpl::getInstance()->createBoolCVar(name, defaultValue, flags, description)->index;
	}

	bool AutoCVar_Bool::get() const
	{
		return getCVarCurrentByIndex<bool>(index);
	}

	bool* AutoCVar_Bool::getPtr() const
	{
		return ptrGetCVarCurrentByIndex<bool>(index);
	}

	void AutoCVar_Bool::set(bool val) const
	{
		setCVarCurrentByIndex<bool>(index, val);
	}





	AutoCVar_String::AutoCVar_String(std::string_view name, std::string& defaultValue, CVarFlags flags, std::string_view description)
	{
		index = CVarSystemImpl::getInstance()->createStringCVar(name, defaultValue, flags, description)->index;
	}
	std::string_view AutoCVar_String::get() const
	{
		return *ptrGetCVarCurrentByIndex<std::string>(index);
	}
	std::string AutoCVar_String::getCopy() const
	{
		return getCVarCurrentByIndex<std::string>(index);
	}
	void AutoCVar_String::set(std::string_view val) const
	{
		setCVarCurrentByIndex<std::string>(index, std::string(val));
	}

	void AutoCVar_String::set(const std::string& val) const
	{
		setCVarCurrentByIndex<std::string>(index, val);
	}


} // namespace utl
