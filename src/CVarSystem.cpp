#include "CVarSystem.h"

#include <iostream>
#include <shared_mutex>
#include <variant>
#include <ranges>
#include <mutex>

#include "FSNavigator.h"
#include "StackedSliceArray.h"
#include "ThirdParty/ini.h"
#include "thirdparty/json.hpp"
using json = nlohmann::json;
namespace utl {


	enum class CVarType
	{
		Int,
		Float,
		String,
		Bool,
		Vec2,
		Vec3,
		Vec4
	};


	//example of what json should store
	// current value, name, description,  flags, type, min, max 

	struct CVarInfo
	{
		std::string name;
		std::string description;
		CVarFlags flags;
		CVarType type;
		CVarMinMaxType min{ std::monostate{} };
		CVarMinMaxType max{ std::monostate{} };
		size_t index;
	};

	std::pair<std::string,std::string> GetCVarInfoNameAndCategory(CVarInfo& info)
	{
		if (auto c = info.name.find_first_of('.'); c != std::string::npos)
		{
			return { info.name.substr(c + 1),info.name.substr(0, c) };
		}
		return { info.name, "" };
	}

	struct CVarProperty
	{
		CVarValue initial;
		CVarValue current;
		CVarInfo* info;
	};



	template <size_t N>
	struct CVarPropertyArray
	{
		std::array<CVarProperty, N> data{};
		size_t lastCVar{ 0 };

		CVarPropertyArray() = default;


		CVarProperty getProperty(size_t index)
		{
			if (index >= data.size()) return {};
			return data[index];
		}
		CVarProperty* getPropertyPtr(size_t index)
		{
			if (index >= data.size()) return {};
			return &data[index];
		}

		CVarValue getCurrent(size_t index)
		{
			if (index >= data.size()) return {};
			return data[index].current;
		}

		CVarValue* getCurrentPtr(size_t index)
		{
			if (index >= data.size()) return nullptr;
			return &data[index].current;
		}



		auto begin() { return data.begin(); }
		auto end() { return data.begin() + lastCVar; }


		void setCurrent(size_t index, const CVarValue& value)
		{
			if (index >= data.size()) return;
			data[index].current = value;
		}

		size_t add(const CVarValue& initial, const CVarValue& current, CVarInfo* info)
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

		size_t add(const CVarValue& value, CVarInfo* info)
		{
			return add(value, value, info);
		}


	};
	namespace {
		constexpr  size_t MAX_CVARS = 2048;
	}

	class CVarSystemImpl : public CVarSystem
	{
	private:
		std::shared_mutex mutex;
		std::unordered_map<uint64_t, CVarInfo> infoMap;
		CVarPropertyArray<MAX_CVARS> properties;

		//friend void from_json(const json& j, utl::CVarSystemImpl& cSys);



		CVarInfo* initCVar(const std::string_view name, const std::string_view description, const  CVarType type, CVarFlags flags, const CVarMinMaxType min = std::monostate{}, const CVarMinMaxType max = std::monostate{})
		{
			//std::unique_lock lock(mutex);
			auto hash = StringHash(name);
			auto& info = infoMap[hash];
			info.name = name;
			info.description = description;
			info.type = type;
			info.flags = CVarFlags::None;
			info.min = min;
			info.max = max;
			return &info;
		}

	public:

		CVarInfo* tryInitCVar(const std::string_view name,CVarValue initial, const std::string_view description, const   CVarType type, CVarFlags flags, const CVarMinMaxType min = std::monostate{}, const CVarMinMaxType max = std::monostate{})
		{
			auto hash = StringHash(name);
			auto& info = infoMap[hash];
			if (info.name.empty()) {
				info.name = name;
				info.description = description;
				info.type = type;
				info.flags = CVarFlags::None;
				info.min = min;
				info.max = max;
				properties.add(initial, &info);
				return &info;
			}
			return nullptr;
		}

		CVarPropertyArray<MAX_CVARS>& getProperties() { return properties; }

		CVarInfo* getCVar(StringHash<> name) override;
		CVarInfo* createFloatCVar(std::string_view name, std::string_view description, 
								  double initial, CVarFlags flags, double min, double max) override;
		CVarInfo* createIntCVar(std::string_view name, std::string_view description, 
								int64_t initial, CVarFlags flags, int64_t min, int64_t max) override;
		CVarInfo* createBoolCVar(std::string_view name, std::string_view description, 
								 bool initial, CVarFlags flags) override;
		CVarInfo* createStringCVar(std::string_view name, std::string_view description, 
								   std::string_view initial, CVarFlags flags) override;
		CVarInfo* createVec2CVar(std::string_view name, std::string_view description, 
								 Vec2 initial, CVarFlags flags, double min, double max) override;
		CVarInfo* createVec3CVar(std::string_view name, std::string_view description, 
								 Vec3 initial, CVarFlags flags, double min, double max) override;
		CVarInfo* createVec4CVar(std::string_view name, std::string_view description, 
								 Vec4 initial, CVarFlags flags, double min, double max) override;
		void setFloatCVar(std::string_view name, double values) override;
		void setIntCVar(std::string_view name, int64_t values) override;
		void setBoolCVar(std::string_view name, bool values) override;
		void setStringCVar(std::string_view name, std::string_view values) override;
		void setVec2CVar(std::string_view name, Vec2 values) override;
		void setVec3CVar(std::string_view name, Vec3 values) override;
		void setVec4CVar(std::string_view name, Vec4 values) override;
		double getFloatCVar(std::string_view name) override;
		int64_t getIntCVar(std::string_view name) override;
		bool getBoolCVar(std::string_view name) override;
		std::string getStringCVar(std::string_view name) override;
		Vec2 getVec2CVar(std::string_view name) override;
		Vec3 getVec3CVar(std::string_view name) override;
		Vec4 getVec4CVar(std::string_view name) override;

		auto begin() { return infoMap.begin(); }
		auto end() { return infoMap.end(); }

		static CVarSystemImpl* Get()
		{
			return dynamic_cast<CVarSystemImpl*>(CVarSystem::Get());
		}

		void renderIGUIDisplay(ICVarDisplay& display) override;

		void saveAsIni(std::string_view filename);
		void loadFromIni(std::string_view filename);

		void saveFile(std::string_view filename) override;;
		void saveFile(FSNavigator& filename) override;;

		void loadFile(std::string_view filename) override;;
		void loadFile(FSNavigator& filename) override;;

		void debugPrint() override;;
	};
}
namespace {
	void to_json(json& j, const utl::CVarFlags& flags) { j = static_cast<uint32_t>(flags); }
	void from_json(const json& j, utl::CVarFlags& flags) { flags = static_cast<utl::CVarFlags>(j.get<uint32_t>()); }
	void to_json(json& j, const utl::CVarValue& value)
	{
		std::visit([&j](auto&& arg) {
			j = arg;
				   }, value);
	}
	void from_json(const json& j, utl::CVarValue& value)
	{
		if (j.is_boolean()) {
			value = j.get<bool>();
		} else if (j.is_number_integer()) {
			value = j.get<int64_t>();
		} else if (j.is_number_float()) {
			value = j.get<double>();
		} else if (j.is_string()) {
			value = j.get<std::string>();
		} else if (j.is_array() && j.size() == 2) {
			value = j.get<std::array<double, 2>>();
		} else if (j.is_array() && j.size() == 3) {
			value = j.get<std::array<double, 3>>();
		} else if (j.is_array() && j.size() == 4) {
			value = j.get<std::array<double, 4>>();
		} else {
			throw std::runtime_error("Unknown type in CVarValue JSON deserialization");
		}
	}
	void from_json(const json& j, utl::CVarValue& value, utl::CVarType& type)
	{
		if (j["current"].is_boolean()) {
			value = j["current"].get<bool>();
			type = utl::CVarType::Bool;
		} else if (j["current"].is_number_integer()) {
			value = j["current"].get<int64_t>();
			type = utl::CVarType::Int;
		} else if (j["current"].is_number_float()) {
			value = j["current"].get<double>();
			type = utl::CVarType::Float;
		} else if (j["current"].is_string()) {
			value = j["current"].get<std::string>();
			type = utl::CVarType::String;
		} else if (j["current"].is_array() && j.size() == 2) {
			value = j["current"].get<std::array<double, 2>>();
			type = utl::CVarType::Vec2;
		} else if (j["current"].is_array() && j.size() == 3) {
			value = j["current"].get<std::array<double, 3>>();
			type = utl::CVarType::Vec3;
		} else if (j["current"].is_array() && j.size() == 4) {
			value = j["current"].get<std::array<double, 4>>();
			type = utl::CVarType::Vec4;
		} else {
			throw std::runtime_error("Unknown type in CVarValue JSON deserialization");
		}
	}
	void to_json(json& j, const utl::CVarMinMaxType& minmax)
	{
		std::visit([&j](auto&& arg) {
			using T = std::decay_t<decltype(arg)>;
			if constexpr (std::is_same_v<T, std::monostate>) {
				j = nullptr;
			} else {
				j = arg;
			}
				   }, minmax);
	}
	void from_json(const json& j, utl::CVarMinMaxType& minmax)
	{
		if (j.is_null()) {
			minmax = std::monostate{};
		} else if (j.is_number_integer()) {
			minmax = j.get<int64_t>();
		} else if (j.is_number_float()) {
			minmax = j.get<double>();
		} else {
			throw std::runtime_error("Unknown type in CVarMinMaxType JSON deserialization");
		}
	}

	void to_json(json& j, utl::CVarSystemImpl& cSys)
	{
		for (auto&& prop : cSys.getProperties()) {
			auto& info = *prop.info;
			json jp;
			jp["description"] = info.description;
			jp["flags"] = info.flags;
			// jp["type"] = info.type;  // Uncomment if needed
			to_json(jp["min"], info.min);
			to_json(jp["max"], info.max);
			to_json(jp["current"], prop.current);

			j[info.name] = jp;  // Assign jp as a nested JSON under info.name
		}
	}
	void from_json(const json& j, utl::CVarSystemImpl& cSys)
	{
		for (auto&& [key, value] : j.items()) {
			utl::CVarValue val;
			utl::CVarType type;
			from_json(value, val, type);
			utl::CVarMinMaxType min;
			from_json(value.at("min"), min);
			utl::CVarMinMaxType max;
			from_json(value.at("max"), max);

			switch (type) {
			case utl::CVarType::Bool:
				cSys.createBoolCVar(key, value.at("description"),  std::get<bool>(val), value.at("flags"));
				break;
			case utl::CVarType::Int:
				cSys.createIntCVar(key, value.at("description"),  std::get<int64_t>(val), value.at("flags"), std::get<int64_t>(min), std::get<int64_t>(max));
				break;
			case utl::CVarType::Float:
				cSys.createFloatCVar(key, value.at("description"),  std::get<double>(val), value.at("flags"), std::get<double>(min), std::get<double>(max));
				break;
			case utl::CVarType::String:
				cSys.createStringCVar(key, value.at("description"),  std::get<std::string>(val), value.at("flags"));
				break;
			case utl::CVarType::Vec2:
				cSys.createVec2CVar(key, value.at("description"),  std::get<std::array<double, 2>>(val), value.at("flags"), std::get<double>(min), std::get<double>(max));
				break;
			case utl::CVarType::Vec3:
				cSys.createVec3CVar(key, value.at("description"),  std::get<std::array<double, 3>>(val), value.at("flags"), std::get<double>(min), std::get<double>(max));
				break;
			case utl::CVarType::Vec4:
				cSys.createVec4CVar(key, value.at("description"),  std::get<std::array<double, 4>>(val), value.at("flags"), std::get<double>(min), std::get<double>(max));
				break;
			}
		}
	}

}
namespace utl {
	CVarInfo* CVarSystemImpl::getCVar(StringHash<> name)
	{
		std::shared_lock lock(mutex);
		const auto it = infoMap.find(name);
		if (it == infoMap.end()) {
			return nullptr;
		}
		return &it->second;
	}

	CVarSystem* CVarSystem::Get()
	{

		static  CVarSystemImpl instance;
		return &instance;
	}

	CVarInfo* CVarSystemImpl::createFloatCVar(const std::string_view name, const std::string_view description,
											  const  double initial, CVarFlags flags, double min, double max)
	{
		std::unique_lock lock(mutex);
		CVarInfo* info = initCVar(name, description,  CVarType::Float, flags, min, max);

		if (!info) return nullptr;

		properties.add(initial, info);

		return info;

	}

	CVarInfo* CVarSystemImpl::createIntCVar(const std::string_view name, const std::string_view description, const 
											int64_t initial, CVarFlags flags, int64_t min, int64_t max)
	{
		std::unique_lock lock(mutex);
		CVarInfo* info = initCVar(name, description,  CVarType::Int, flags, min, max);

		if (!info) return nullptr;

		properties.add(initial, info);

		return info;

	}

	CVarInfo* CVarSystemImpl::createBoolCVar(const std::string_view name, const std::string_view description, const 
											 bool initial, CVarFlags flags)
	{
		std::unique_lock lock(mutex);
		CVarInfo* info = initCVar(name, description,  CVarType::Bool, flags);

		if (!info) return nullptr;

		properties.add(initial, info);

		return info;
	}

	CVarInfo* CVarSystemImpl::createStringCVar(const std::string_view name, const std::string_view description,
											   const std::string_view initial, CVarFlags flags)
	{
		std::unique_lock lock(mutex);
		CVarInfo* info = initCVar(name, description,  CVarType::String, flags);

		if (!info) return nullptr;

		properties.add(std::string(initial), info);

		return info;
	}

	CVarInfo* CVarSystemImpl::createVec2CVar(const std::string_view name, const std::string_view description, const 
											 Vec2 initial, CVarFlags flags, double min, double max)
	{
		std::unique_lock lock(mutex);
		CVarInfo* info = initCVar(name, description,  CVarType::Float, flags, min, max);

		if (!info) return nullptr;

		properties.add(initial, info);

		return info;
	}

	CVarInfo* CVarSystemImpl::createVec3CVar(const std::string_view name, const std::string_view description, const 
											 Vec3 initial, CVarFlags flags, double min, double max)
	{
		std::unique_lock lock(mutex);
		CVarInfo* info = initCVar(name, description,  CVarType::Float, flags, min, max);

		if (!info) return nullptr;

		properties.add(initial, info);

		return info;
	}

	CVarInfo* CVarSystemImpl::createVec4CVar(const std::string_view name, const std::string_view description, const 
											 Vec4 initial, CVarFlags flags, double min, double max)
	{
		std::unique_lock lock(mutex);
		CVarInfo* info = initCVar(name, description,  CVarType::Float, flags, min, max);

		if (!info) return nullptr;

		properties.add(initial, info);

		return info;
	}

	void CVarSystemImpl::setFloatCVar(const std::string_view name, double values)
	{
		const auto info = getCVar(StringHash(name));
		if (!info) return;
		const auto index = info->index;
		properties.setCurrent(index, values);
	}

	void CVarSystemImpl::setIntCVar(const std::string_view name, int64_t values)
	{
		const auto info = getCVar(StringHash(name));
		if (!info) return;
		const auto index = info->index;
		properties.setCurrent(index, values);
	}

	void CVarSystemImpl::setBoolCVar(const std::string_view name, bool values)
	{
		const auto info = getCVar(StringHash(name));
		if (!info) return;
		const auto index = info->index;
		properties.setCurrent(index, values);
	}

	void CVarSystemImpl::setStringCVar(const std::string_view name, const std::string_view values)
	{
		const auto info = getCVar(StringHash(name));
		if (!info) return;
		const auto index = info->index;
		properties.setCurrent(index, std::string(values));
	}

	void CVarSystemImpl::setVec2CVar(const std::string_view name, Vec2 values)
	{
		const auto info = getCVar(StringHash(name));
		if (!info) return;
		const auto index = info->index;
		properties.setCurrent(index, values);
	}

	void CVarSystemImpl::setVec3CVar(const std::string_view name, Vec3 values)
	{
		const auto info = getCVar(StringHash(name));
		if (!info) return;
		const auto index = info->index;
		properties.setCurrent(index, values);
	}

	void CVarSystemImpl::setVec4CVar(const std::string_view name, Vec4 values)
	{
		const auto info = getCVar(StringHash(name));
		if (!info) return;
		const auto index = info->index;
		properties.setCurrent(index, values);
	}

	double CVarSystemImpl::getFloatCVar(const std::string_view name)
	{
		const auto info = getCVar(StringHash(name));
		if (!info) return 0.0;
		const auto index = info->index;
		return std::get<double>(properties.getCurrent(index));
	}

	int64_t CVarSystemImpl::getIntCVar(const std::string_view name)
	{
		const auto info = getCVar(StringHash(name));
		if (!info) return 0;
		const auto index = info->index;
		return std::get<int64_t>(properties.getCurrent(index));
	}

	bool CVarSystemImpl::getBoolCVar(const std::string_view name)
	{
		const auto info = getCVar(StringHash(name));
		if (!info) return false;
		const auto index = info->index;
		return std::get<bool>(properties.getCurrent(index));
	}

	std::string CVarSystemImpl::getStringCVar(const std::string_view name)
	{
		const auto info = getCVar(StringHash(name));
		if (!info) return "";
		const auto index = info->index;
		return std::get<std::string>(properties.getCurrent(index));
	}

	CVarSystem::Vec2 CVarSystemImpl::getVec2CVar(const std::string_view name)
	{
		const auto info = getCVar(StringHash(name));
		if (!info) return {};
		const auto index = info->index;
		return std::get<Vec2>(properties.getCurrent(index));
	}

	CVarSystem::Vec3 CVarSystemImpl::getVec3CVar(const std::string_view name)
	{
		const auto info = getCVar(StringHash(name));
		if (!info) return {};
		const auto index = info->index;
		return std::get<Vec3>(properties.getCurrent(index));
	}

	CVarSystem::Vec4 CVarSystemImpl::getVec4CVar(const std::string_view name)
	{
		const auto info = getCVar(StringHash(name));
		if (!info) return {};
		const auto index = info->index;
		return std::get<Vec4>(properties.getCurrent(index));
	}

	void CVarSystemImpl::renderIGUIDisplay(ICVarDisplay& display)
	{

		for (auto&& info : infoMap | std::views::values) {
			auto&& property = properties.getProperty(info.index);

			display.display(property.initial, property.current, info.name, info.description, info.flags,
							info.min, info.max);



		}
	}

	void CVarSystemImpl::saveAsIni(std::string_view filename)
	{
		std::string filenameStr = std::string(filename);
		if (filenameStr.find(".ini") == std::string::npos) {
			filenameStr += ".ini";
		}
		mINI::INIFile file{ filenameStr };
		mINI::INIStructure ini;

		file.read(ini);

		//iterate through infomap and save the values to the file
		for (auto&& p : infoMap) {
			auto& info = p.second;
			auto index = info.index;
			CVarProperty& property = *properties.getPropertyPtr(index);
			std::string value;


			auto [name, category] = GetCVarInfoNameAndCategory(info);

			std::visit([&](auto&& arg) {
				using T = std::decay_t<decltype(arg)>;
				if constexpr (std::is_same_v<T, bool>)
					value = arg ? "true" : "false";
				else if constexpr (std::is_same_v<T, int64_t>)
					value = std::to_string(arg);
				else if constexpr (std::is_same_v<T, double>)
					value = std::to_string(arg);
				else if constexpr (std::is_same_v<T, std::array<double, 2>>)
					value = std::to_string(arg[0]) + ", " + std::to_string(arg[1]);
				else if constexpr (std::is_same_v<T, std::array<double, 3>>)
					value = std::to_string(arg[0]) + ", " + std::to_string(arg[1]) + ", " + std::to_string(arg[2]);
				else if constexpr (std::is_same_v<T, std::array<double, 4>>)
					value = std::to_string(arg[0]) + ", " + std::to_string(arg[1]) + ", " + std::to_string(arg[2]) + ", " + std::to_string(arg[3]);
				else if constexpr (std::is_same_v<T, std::string>)
					value = arg;
					   }, property.current);

			ini[category][name] = value;
		}
		file.write(ini);


	}
	void CVarSystemImpl::loadFromIni(std::string_view filename)
	{

		std::string filenameStr = std::string(filename);
		if (filenameStr.find(".ini") == std::string::npos) {
			filenameStr += ".ini";
		}
		mINI::INIFile file{ filenameStr };
		mINI::INIStructure ini;

		if (!file.read(ini)) {
			throw std::runtime_error("Could not open or read INI file");
		}

		// Iterate through the INI structure
		for (const auto& [category, keys] : ini) {
			for (const auto& [name, valueStr] : keys) {
				// Parse and deduce the value type, then create the property
				utl::CVarValue val;
				utl::CVarType type;
				utl::CVarMinMaxType min, max;

				// Deduce type and assign value
				if (valueStr == "true" || valueStr == "false") {
					val = (valueStr == "true");
					type = utl::CVarType::Bool;
					this->createBoolCVar( std::string(category).append(".")+ name, "",  std::get<bool>(val), {});
				} else if (std::count(valueStr.begin(), valueStr.end(), ',') > 0) {
					std::istringstream ss(valueStr);
					std::vector<double> values;
					std::string item;
					while (std::getline(ss, item, ',')) {
						values.push_back(std::stod(item));
					}

					// Check vector size to determine Vec2, Vec3, or Vec4
					if (values.size() == 2) {
						val = std::array<double, 2>{values[0], values[1]};
						type = utl::CVarType::Vec2;
						this->createVec2CVar(std::string(category).append(".") + name, "",  std::get<std::array<double, 2>>(val), {}, std::numeric_limits<double>::min(), std::numeric_limits<double>::max());  // replace with actual min/max if applicable
					} else if (values.size() == 3) {
						val = std::array<double, 3>{values[0], values[1], values[2]};
						type = utl::CVarType::Vec3;
						this->createVec3CVar(std::string(category).append(".") + name, "",  std::get<std::array<double, 3>>(val), {}, std::numeric_limits<double>::min(), std::numeric_limits<double>::max());
					} else if (values.size() == 4) {
						val = std::array<double, 4>{values[0], values[1], values[2], values[3]};
						type = utl::CVarType::Vec4;
						this->createVec4CVar(std::string(category).append(".") + name, "",  std::get<std::array<double, 4>>(val), {}, std::numeric_limits<double>::min(), std::numeric_limits<double>::max());
					}
				} else if (valueStr.find('.') != std::string::npos) {
					val = std::stod(valueStr);
					type = utl::CVarType::Float;
					this->createFloatCVar(std::string(category).append(".") + name, "",  std::get<double>(val), {}, std::numeric_limits<double>::min(), std::numeric_limits<double>::max());
				} else {
					try {
						val = std::stoll(valueStr);
						type = utl::CVarType::Int;
						this->createIntCVar(std::string(category).append(".") + name, "",  std::get<int64_t>(val), {}, std::numeric_limits<int64_t>::min(), std::numeric_limits<int64_t>::max());  // replace with actual min/max if applicable
					} catch (...) {
						val = valueStr;
						type = utl::CVarType::String;
						this->createStringCVar(std::string(category).append(".") + name, "",  std::get<std::string>(val), {});
					}
				}
			}
		}
	}

	bool iniOrJson(std::string_view filename)
	{
		return filename.find(".ini") != std::string::npos;
	}
	void CVarSystemImpl::saveFile(std::string_view filename)
	{
		//handle mutex
		std::shared_lock lock(mutex);
		bool isIni = iniOrJson(filename);
		if (isIni) {
			saveAsIni(filename);
			return;
		}
		std::string filenameStr = std::string(filename);
		if (filenameStr.find(".json") == std::string::npos) {
			filenameStr += ".json";
		}
		json j;
		CVarSystemImpl& cvSys = *this;
		to_json(j, cvSys);

		std::ofstream file{ filenameStr };
		if (!file.is_open()) {
			throw std::runtime_error("Could not open file for writing");
		}
		file << j.dump(4);
	}

	void CVarSystemImpl::saveFile(FSNavigator& filename)
	{
		auto path = filename.getFilePath(filename.getSelectedFile().value_or("")).string();
		saveFile(path);

	}

	void CVarSystemImpl::loadFile(std::string_view filename)
	{
		//std::unique_lock lock(mutex);
		if (bool isIni = iniOrJson(filename)) {
			loadFromIni(filename);
			return;
		}
		std::string filenameStr = std::string(filename);
		if (filenameStr.find(".json") == std::string::npos) {
			filenameStr += ".json";
		}
		json j;
		std::ifstream file{ filenameStr };
		if (!file.is_open()) {
			throw std::runtime_error("Could not open file for reading");
		}
		file >> j;
		CVarSystemImpl& cvSys = *this;
		from_json(j, cvSys);


	}

	void CVarSystemImpl::loadFile(FSNavigator& filename)
	{

		auto path = filename.getFilePath(filename.getSelectedFile().value_or("")).string();
		loadFile(path);

	}

	void CVarSystemImpl::debugPrint()
	{
		std::shared_lock lock(mutex);
		for (const auto& property : properties) {
			auto& info = *property.info;
			auto nameAndCategory = GetCVarInfoNameAndCategory(info);
			std::cout << "Property [" << nameAndCategory.second << "][" << nameAndCategory.first<< "] = ";

			std::visit([](auto&& arg) {
				using T = std::decay_t<decltype(arg)>;
				if constexpr (std::is_same_v<T, bool>)
					std::cout << (arg ? "true" : "false");
				else if constexpr (std::is_same_v<T, int64_t>)
					std::cout << arg;
				else if constexpr (std::is_same_v<T, double>)
					std::cout << arg;
				else if constexpr (std::is_same_v<T, std::array<double, 2>>)
					std::cout << "Vec2(" << arg[0] << ", " << arg[1] << ")";
				else if constexpr (std::is_same_v<T, std::array<double, 3>>)
					std::cout << "Vec3(" << arg[0] << ", " << arg[1] << ", " << arg[2] << ")";
				else if constexpr (std::is_same_v<T, std::array<double, 4>>)
					std::cout << "Vec4(" << arg[0] << ", " << arg[1] << ", " << arg[2] << ", " << arg[3] << ")";
				else if constexpr (std::is_same_v<T, std::string>)
					std::cout << arg;
					   }, property.current);

			std::cout << std::endl;
		}
	}


	AutoCVar_Float::AutoCVar_Float(std::string_view name, double initial,  std::string_view description,
								   CVarFlags flags, double min, double max)
	{
		auto& cvSys = *CVarSystemImpl::Get();
		auto* info = cvSys.getCVar(name);
		if (info) {
			index = info->index;
			return;
		}
		index = cvSys.createFloatCVar(name, description,  initial, flags, min, max)->index;

	}

	double AutoCVar_Float::get() const
	{
		return std::get<double>(CVarSystemImpl::Get()->getProperties().getCurrent(index));
	}

	double* AutoCVar_Float::getPtr() const
	{
		auto&& gotten = (CVarSystemImpl::Get()->getProperties().getCurrentPtr(index));
		if (!gotten) return nullptr;
		return  &std::get<double>(*gotten);
	}

	float AutoCVar_Float::getFloat() const
	{
		return	static_cast<float>(get());
	}

	float* AutoCVar_Float::getFloatPtr() const
	{
		return reinterpret_cast<float*>(getPtr());
	}

	void AutoCVar_Float::set(double val) const
	{
		CVarSystemImpl::Get()->getProperties().setCurrent(index, val);
	}

	AutoCVar_Int::AutoCVar_Int(std::string_view name, int64_t initial,  std::string_view description,
							   CVarFlags flags, int64_t min, int64_t max)
	{
		auto& cvSys = *CVarSystemImpl::Get();
		if (auto* info = cvSys.getCVar(name)) {
			index = info->index;
			if (info->type != utl::CVarType::Int) {
				throw std::runtime_error("CVar type mismatch");
			}
			const auto flag = static_cast<int64_t>(info->flags);
			const auto isReadOnly = flag & static_cast<int64_t>(CVarFlags::EditReadOnly);
			const auto isDontOverwriteOnLoad = flag & static_cast<int64_t>(CVarFlags::DontOverwriteOnLoad);

			if (!isReadOnly || !isDontOverwriteOnLoad )
			{
				cvSys.setIntCVar(name, initial);
			}
			return;
		}
		index = cvSys.createIntCVar(name, description,  initial, flags, min, max)->index;
	}

	AutoCVar_Bool::AutoCVar_Bool(std::string_view name, bool initial,  std::string_view description,
								 CVarFlags flags)
	{
		auto& cvSys = *CVarSystemImpl::Get();
		auto* info = cvSys.getCVar(name);
		if (info) {
			index = info->index;
			return;
		}
		index = cvSys.createBoolCVar(name, description,  initial, flags)->index;
	}

	bool AutoCVar_Bool::get() const
	{
		return std::get<bool>(CVarSystemImpl::Get()->getProperties().getCurrent(index));
	}

	bool* AutoCVar_Bool::getPtr() const
	{
		auto&& gotten = (CVarSystemImpl::Get()->getProperties().getCurrentPtr(index));
		if (!gotten) return nullptr;
		return  &std::get<bool>(*gotten);
	}

	void AutoCVar_Bool::set(bool val) const
	{
		CVarSystemImpl::Get()->getProperties().setCurrent(index, val);
	}

	AutoCVar_String::AutoCVar_String(std::string_view name, std::string_view initial,  std::string_view description, CVarFlags flags)
	{
		auto& cvSys = *CVarSystemImpl::Get();
		auto* info = cvSys.getCVar(name);
		if (info) {
			index = info->index;
			return;
		}
		index = cvSys.createStringCVar(name, description,  initial, flags)->index;
	}

	std::string_view AutoCVar_String::get() const
	{
		return std::get<std::string>(CVarSystemImpl::Get()->getProperties().getCurrent(index));
	}

	void AutoCVar_String::set(std::string_view val) const
	{
		CVarSystemImpl::Get()->getProperties().setCurrent(index, std::string(val));
	}

	AutoCVar_Vec2::AutoCVar_Vec2(std::string_view name, CVarSystem::Vec2 initial,  std::string_view description,
								 CVarFlags flags, double min, double max)
	{
		auto& cvSys = *CVarSystemImpl::Get();
		auto* info = cvSys.getCVar(name);
		if (info) {
			index = info->index;
			return;
		}
		index = cvSys.createVec2CVar(name, description,  initial, flags, min, max)->index;
	}


	CVarSystem::Vec2 AutoCVar_Vec2::get()const
	{
		return std::get<CVarSystem::Vec2>(CVarSystemImpl::Get()->getProperties().getCurrent(index));
	}

	CVarSystem::Vec2* AutoCVar_Vec2::getPtr()const
	{
		auto&& gotten = (CVarSystemImpl::Get()->getProperties().getCurrentPtr(index));
		if (!gotten) return nullptr;
		return  &std::get<CVarSystem::Vec2>(*gotten);
	}

	void AutoCVar_Vec2::set(CVarSystem::Vec2 val)const
	{
		CVarSystemImpl::Get()->getProperties().setCurrent(index, val);
	}

	AutoCVar_Vec3::AutoCVar_Vec3(std::string_view name, const CVarSystem::Vec3& initial,  std::string_view description,
								 CVarFlags flags, double min, double max)
	{
		auto& cvSys = *CVarSystemImpl::Get();
		auto* info = cvSys.getCVar(name);
		if (info) {
			index = info->index;
			return;
		}
		index = cvSys.createVec3CVar(name, description,  initial, flags, min, max)->index;
	}

	AutoCVar_Vec4::AutoCVar_Vec4(std::string_view name, const CVarSystem::Vec4& initial,  std::string_view description,
								 CVarFlags flags, double min, double max)
	{
		auto& cvSys = *CVarSystemImpl::Get();
		auto* info = cvSys.getCVar(name);
		if (info) {
			index = info->index;
			return;
		}
		index = cvSys.createVec4CVar(name, description,  initial, flags, min, max)->index;
	}

	CVarSystem::Vec4 AutoCVar_Vec4::get()const
	{
		return std::get<CVarSystem::Vec4>(CVarSystemImpl::Get()->getProperties().getCurrent(index));
	}


	CVarSystem::Vec4* AutoCVar_Vec4::getPtr()const
	{
		auto&& gotten = (CVarSystemImpl::Get()->getProperties().getCurrentPtr(index));
		if (!gotten) return nullptr;
		return  &std::get<CVarSystem::Vec4>(*gotten);
	}

	void AutoCVar_Vec4::set(CVarSystem::Vec4 val)const
	{
		CVarSystemImpl::Get()->getProperties().setCurrent(index, val);
	}

	CVarSystem::Vec3 AutoCVar_Vec3::get()const
	{
		return std::get<CVarSystem::Vec3>(CVarSystemImpl::Get()->getProperties().getCurrent(index));
	}

	CVarSystem::Vec3* AutoCVar_Vec3::getPtr()const
	{
		auto&& gotten = (CVarSystemImpl::Get()->getProperties().getCurrentPtr(index));
		if (!gotten) return nullptr;
		return  &std::get<CVarSystem::Vec3>(*gotten);
	}

	void AutoCVar_Vec3::set(CVarSystem::Vec3 val)const
	{
		CVarSystemImpl::Get()->getProperties().setCurrent(index, val);
	}

	int64_t AutoCVar_Int::get()const
	{
		return std::get<int64_t>(CVarSystemImpl::Get()->getProperties().getCurrent(index));
	}

	int64_t* AutoCVar_Int::getPtr()const
	{
		auto&& gotten = (CVarSystemImpl::Get()->getProperties().getCurrentPtr(index));
		if (!gotten) return nullptr;
		return  &std::get<int64_t>(*gotten);
	}

	void AutoCVar_Int::set(int64_t val)const
	{
		CVarSystemImpl::Get()->getProperties().setCurrent(index, val);
	}
} // namespace utl