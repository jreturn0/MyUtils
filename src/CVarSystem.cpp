#include "CVarSystem.h"
#include <shared_mutex>
#include <variant>
#include <ranges>
#include "StackedSliceArray.h"

enum class Type
{
	Int,
	Float,
	String,
	Bool,
	Vec2,
	Vec3,
	Vec4
};


struct CVarInfo
{
	std::string name;
	std::string description;
	std::string category;
	CVarFlags flags;
	Type type;
	CVarMinMaxType min{ std::monostate{} };
	CVarMinMaxType max{ std::monostate{} };
	size_t index;
};

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



class CVarSystemImpl : public CVarSystem
{
private:
	std::shared_mutex mutex;
	std::unordered_map<uint64_t, CVarInfo> infoMap;
	constexpr static size_t MAX_CVARS = 2048;
	CVarPropertyArray<MAX_CVARS> properties;

	CVarInfo* initCVar(const std::string_view name, const std::string_view description, const std::string_view category, const Type type, CVarFlags flags, const CVarMinMaxType min = std::monostate{}, const CVarMinMaxType max = std::monostate{})
	{



		//std::unique_lock lock(mutex);
		auto hash = StringHash(name);
		auto& info = infoMap[hash];
		info.name = name;
		info.description = description;
		info.category = category;
		info.type = type;
		info.flags = CVarFlags::None;
		info.min = min;
		info.max = max;
		return &info;
	}

public:

	CVarPropertyArray<MAX_CVARS>& getProperties() { return properties; }

	CVarInfo* getCVar(StringHash<> name) override;
	CVarInfo* createFloatCVar(std::string_view name, std::string_view description, std::string_view category,
							  double initial, CVarFlags flags, double min, double max) override;
	CVarInfo* createIntCVar(std::string_view name, std::string_view description, std::string_view category,
							int64_t initial, CVarFlags flags, int64_t min, int64_t max) override;
	CVarInfo* createBoolCVar(std::string_view name, std::string_view description, std::string_view category,
							 bool initial, CVarFlags flags) override;
	CVarInfo* createStringCVar(std::string_view name, std::string_view description, std::string_view category,
							   std::string_view initial, CVarFlags flags) override;
	CVarInfo* createVec2CVar(std::string_view name, std::string_view description, std::string_view category,
							 Vec2 initial, CVarFlags flags, double min, double max) override;
	CVarInfo* createVec3CVar(std::string_view name, std::string_view description, std::string_view category,
							 Vec3 initial, CVarFlags flags, double min, double max) override;
	CVarInfo* createVec4CVar(std::string_view name, std::string_view description, std::string_view category,
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

	static CVarSystemImpl* Get()
	{
		return dynamic_cast<CVarSystemImpl*>(CVarSystem::Get());
	}

	void renderIGUIDisplay(ICVarDisplay& display) override;
};

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
										  const std::string_view category, double initial, CVarFlags flags, double min, double max)
{
	std::unique_lock lock(mutex);
	CVarInfo* info = initCVar(name, description, category, Type::Float, flags, min, max);

	if (!info) return nullptr;

	properties.add(initial, info);

	return info;

}

CVarInfo* CVarSystemImpl::createIntCVar(const std::string_view name, const std::string_view description, const std::string_view category,
										int64_t initial, CVarFlags flags, int64_t min, int64_t max)
{
	std::unique_lock lock(mutex);
	CVarInfo* info = initCVar(name, description, category, Type::Int, flags, min, max);

	if (!info) return nullptr;

	properties.add(initial, info);

	return info;

}

CVarInfo* CVarSystemImpl::createBoolCVar(const std::string_view name, const std::string_view description, const std::string_view category,
										 bool initial, CVarFlags flags)
{
	std::unique_lock lock(mutex);
	CVarInfo* info = initCVar(name, description, category, Type::Bool, flags);

	if (!info) return nullptr;

	properties.add(initial, info);

	return info;
}

CVarInfo* CVarSystemImpl::createStringCVar(const std::string_view name, const std::string_view description,
										   const std::string_view category, const std::string_view initial, CVarFlags flags)
{
	std::unique_lock lock(mutex);
	CVarInfo* info = initCVar(name, description, category, Type::String, flags);

	if (!info) return nullptr;

	properties.add(std::string(initial), info);

	return info;
}

CVarInfo* CVarSystemImpl::createVec2CVar(const std::string_view name, const std::string_view description, const std::string_view category,
										 Vec2 initial, CVarFlags flags, double min, double max)
{
	std::unique_lock lock(mutex);
	CVarInfo* info = initCVar(name, description, category, Type::Float, flags, min, max);

	if (!info) return nullptr;

	properties.add(initial, info);

	return info;
}

CVarInfo* CVarSystemImpl::createVec3CVar(const std::string_view name, const std::string_view description, const std::string_view category,
										 Vec3 initial, CVarFlags flags, double min, double max)
{
	std::unique_lock lock(mutex);
	CVarInfo* info = initCVar(name, description, category, Type::Float, flags, min, max);

	if (!info) return nullptr;

	properties.add(initial, info);

	return info;
}

CVarInfo* CVarSystemImpl::createVec4CVar(const std::string_view name, const std::string_view description, const std::string_view category,
										 Vec4 initial, CVarFlags flags, double min, double max)
{
	std::unique_lock lock(mutex);
	CVarInfo* info = initCVar(name, description, category, Type::Float, flags, min, max);

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

		display.display(property.initial, property.current, info.name, info.description, info.category, info.flags,
						info.min, info.max);



	}
}


AutoCVar_Float::AutoCVar_Float(std::string_view name, std::string_view description, std::string_view category,
							   double initial, CVarFlags flags, double min, double max)
{
	index = CVarSystemImpl::Get()->createFloatCVar(name, description, category, initial, flags, min, max)->index;

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

AutoCVar_Int::AutoCVar_Int(std::string_view name, std::string_view description, std::string_view category,
						   int64_t initial, CVarFlags flags, int64_t min, int64_t max)
{
	index = CVarSystemImpl::Get()->createIntCVar(name, description, category, initial, flags, min, max)->index;
}

AutoCVar_Bool::AutoCVar_Bool(std::string_view name, std::string_view description, std::string_view category,
							 bool initial, CVarFlags flags)
{
	index = CVarSystemImpl::Get()->createBoolCVar(name, description, category, initial, flags)->index;
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

AutoCVar_String::AutoCVar_String(std::string_view name, std::string_view description, std::string_view category,
								 std::string_view initial, CVarFlags flags)
{
	index = CVarSystemImpl::Get()->createStringCVar(name, description, category, initial, flags)->index;
}

std::string_view AutoCVar_String::get() const
{
	return std::get<std::string>(CVarSystemImpl::Get()->getProperties().getCurrent(index));
}

void AutoCVar_String::set(std::string_view val) const
{
	CVarSystemImpl::Get()->getProperties().setCurrent(index, std::string(val));
}

AutoCVar_Vec2::AutoCVar_Vec2(std::string_view name, std::string_view description, std::string_view category,
							 CVarSystem::Vec2 initial, CVarFlags flags, double min, double max)
{
	index = CVarSystemImpl::Get()->createVec2CVar(name, description, category, initial, flags, min, max)->index;
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

AutoCVar_Vec3::AutoCVar_Vec3(std::string_view name, std::string_view description, std::string_view category,
							 const CVarSystem::Vec3& initial, CVarFlags flags, double min, double max)
{
	index = CVarSystemImpl::Get()->createVec3CVar(name, description, category, initial, flags, min, max)->index;
}

AutoCVar_Vec4::AutoCVar_Vec4(std::string_view name, std::string_view description, std::string_view category,
							 const CVarSystem::Vec4& initial, CVarFlags flags, double min, double max)
{
	index = CVarSystemImpl::Get()->createVec4CVar(name, description, category, initial, flags, min, max)->index;
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
