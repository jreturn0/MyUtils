#pragma once

#include <string_view>
#include <array>
#include <variant>

#include "StringHash.h"

enum class CVarFlags : uint32_t
{
	None = 0,
	NoEdit = 1 << 1,
	EditReadOnly = 1 << 2,
	Advanced = 1 << 3,
};

struct CVarInfo;



using CVarValue = std::variant<bool, double, std::array<double, 2>, std::array<double, 3>, std::array<double, 4>, int64_t, std::string>;
using CVarMinMaxType = std::variant<int64_t, double, std::monostate>;



class ICVarDisplay
{
public:
	virtual ~ICVarDisplay() = default;

	virtual void display(const CVarValue& initial, CVarValue& current,
						 std::string_view name, std::string_view description, std::string_view category,
						 CVarFlags flags,
						 CVarMinMaxType min, CVarMinMaxType max) = 0;
};

class CVarSystem
{
public:
	using Vec2 = std::array<double, 2>;
	using Vec3 = std::array<double, 3>;
	using Vec4 = std::array<double, 4>;

	virtual ~CVarSystem() = default;

	static CVarSystem* Get();

	virtual  CVarInfo* getCVar(StringHash<> name) = 0;




	virtual void renderIGUIDisplay(ICVarDisplay& display) = 0;


	virtual CVarInfo* createFloatCVar(std::string_view name, std::string_view description, std::string_view category,
									  double initial, CVarFlags flags = CVarFlags::None,
									  double min = std::numeric_limits<double>::min(), double max = std::numeric_limits<double>::max()) = 0;
	virtual CVarInfo* createIntCVar(std::string_view name, std::string_view description, std::string_view category,
									int64_t initial, CVarFlags flags = CVarFlags::None,
									int64_t min = std::numeric_limits<int64_t>::min(),
									int64_t max = std::numeric_limits<int64_t>::max()) = 0;
	virtual CVarInfo* createBoolCVar(std::string_view name, std::string_view description, std::string_view category,
									 bool initial, CVarFlags flags = CVarFlags::None) = 0;
	virtual CVarInfo* createStringCVar(std::string_view name, std::string_view description, std::string_view category,
									   std::string_view initial, CVarFlags flags = CVarFlags::None) = 0;

	virtual CVarInfo* createVec2CVar(std::string_view name, std::string_view description, std::string_view category,
									 Vec2 initial, CVarFlags flags = CVarFlags::None,
									 double min = std::numeric_limits<double>::min(), double max = std::numeric_limits<double>::max()) = 0;
	virtual CVarInfo* createVec3CVar(std::string_view name, std::string_view description, std::string_view category,
									 Vec3 initial, CVarFlags flags = CVarFlags::None,
									 double min = std::numeric_limits<double>::min(), double max = std::numeric_limits<double>::max()) = 0;

	virtual CVarInfo* createVec4CVar(std::string_view name, std::string_view description, std::string_view category,
									 Vec4 initial, CVarFlags flags = CVarFlags::None,
									 double min = std::numeric_limits<double>::min(), double max = std::numeric_limits<double>::max()) = 0;

	virtual void setFloatCVar(std::string_view name, double values) = 0;
	virtual void setIntCVar(std::string_view name, int64_t values) = 0;
	virtual void setBoolCVar(std::string_view name, bool values) = 0;
	virtual void setStringCVar(std::string_view name, std::string_view values) = 0;
	virtual void setVec2CVar(std::string_view name, Vec2 values) = 0;
	virtual void setVec3CVar(std::string_view name, Vec3 values) = 0;
	virtual void setVec4CVar(std::string_view name, Vec4 values) = 0;

	virtual double getFloatCVar(std::string_view name) = 0;
	virtual int64_t getIntCVar(std::string_view name) = 0;
	virtual bool getBoolCVar(std::string_view name) = 0;
	virtual std::string getStringCVar(std::string_view name) = 0;
	virtual Vec2 getVec2CVar(std::string_view name) = 0;
	virtual Vec3 getVec3CVar(std::string_view name) = 0;
	virtual Vec4 getVec4CVar(std::string_view name) = 0;
};

template<typename T>
struct AutoCVar
{
protected:
	size_t index{ };
	using CVarType = T;
};

struct AutoCVar_Float : AutoCVar<double>
{
	AutoCVar_Float(std::string_view name, std::string_view description, std::string_view category,
				   double initial, CVarFlags flags = CVarFlags::None,
				   double min = std::numeric_limits<double>::min(), double max = std::numeric_limits<double>::max());

	double get() const;
	double* getPtr() const;
	float getFloat() const;
	float* getFloatPtr() const;
	void set(double val) const;
};

struct AutoCVar_Int : AutoCVar<int64_t>
{
	AutoCVar_Int(std::string_view name, std::string_view description, std::string_view category,
				 int64_t initial, CVarFlags flags = CVarFlags::None,
				 int64_t min = std::numeric_limits<int64_t>::min(),
				 int64_t max = std::numeric_limits<int64_t>::max());

	int64_t get() const;
	int64_t* getPtr() const;
	void set(int64_t val) const;
};

struct AutoCVar_Bool : AutoCVar<bool>
{
	AutoCVar_Bool(std::string_view name, std::string_view description, std::string_view category,
				  bool initial, CVarFlags flags = CVarFlags::None);

	bool get() const;
	bool* getPtr() const;
	void set(bool val) const;
};

struct AutoCVar_String : AutoCVar<std::string>
{
	AutoCVar_String(std::string_view name, std::string_view description, std::string_view category,
					std::string_view initial, CVarFlags flags = CVarFlags::None);

	std::string_view get() const;
	void set(std::string_view val) const;
};

struct AutoCVar_Vec2 : AutoCVar<CVarSystem::Vec2>
{
	AutoCVar_Vec2(std::string_view name, std::string_view description, std::string_view category,
				  CVarSystem::Vec2 initial, CVarFlags flags = CVarFlags::None,
				  double min = std::numeric_limits<double>::min(), double max = std::numeric_limits<double>::max());

	CVarSystem::Vec2 get() const;
	CVarSystem::Vec2* getPtr() const;
	void set(CVarSystem::Vec2 val) const;
};

struct AutoCVar_Vec3 : AutoCVar<CVarSystem::Vec3>
{
	AutoCVar_Vec3(std::string_view name, std::string_view description, std::string_view category,
				  const CVarSystem::Vec3& initial, CVarFlags flags = CVarFlags::None,
				  double min = std::numeric_limits<double>::min(), double max = std::numeric_limits<double>::max());

	CVarSystem::Vec3 get() const;
	CVarSystem::Vec3* getPtr() const;
	void set(CVarSystem::Vec3 val) const;
};

struct AutoCVar_Vec4 : AutoCVar<CVarSystem::Vec4>
{
	AutoCVar_Vec4(std::string_view name, std::string_view description, std::string_view category,
				  const CVarSystem::Vec4& initial, CVarFlags flags = CVarFlags::None,
				  double min = std::numeric_limits<double>::min(), double max = std::numeric_limits<double>::max());

	CVarSystem::Vec4 get() const;
	CVarSystem::Vec4* getPtr() const;
	void set(CVarSystem::Vec4 val) const;
};
