#pragma once

#include <string_view>
#include <array>
#include <variant>
#include <stdint.h>
#include "StringHash.h"
#include <limits>
#include <string>
class FSNavigator;

namespace utl {

	using CVarFlags = uint64_t;
	namespace CVarFlagBits {
		constexpr uint64_t None = 0;
		constexpr uint64_t Immutable = 1 << 0;          // Cannot be modified after initial set
		constexpr uint64_t ReadOnly = 1 << 1;           // Can be read but not edited
		constexpr uint64_t Hidden = 1 << 2;             // Hidden in standard user interfaces or lists
		constexpr uint64_t DeveloperOnly = 1 << 3;      // Restricted to developer or debug modes
		constexpr uint64_t Volatile = 1 << 4;           // Temporary value, not saved to disk
		constexpr uint64_t Persistent = 1 << 5;         // Saved and restored across sessions
		constexpr uint64_t NoAutoLoad = 1 << 6;			// Loaded manually, not automatically at startup
		constexpr uint64_t Archived = 1 << 7;           // Retained in history or logs for auditing
		constexpr uint64_t SystemCritical = 1 << 8;     // Essential; may impact stability if modified
		constexpr uint64_t Experimental = 1 << 9;       // Marked as unstable or experimental
		constexpr uint64_t RequiresRestart = 1 << 10;   // Changes take effect only after restart
		constexpr uint64_t Default = 1 << 11;			// Default value
		constexpr uint64_t DefaultArg = Default | Persistent;
	}

	struct CVarInfo;



	using CVarValue = std::variant<bool, double, std::array<double, 2>, std::array<double, 3>, std::array<double, 4>, int64_t, std::string>;
	using CVarMinMaxType = std::variant<int64_t, double, std::monostate>;



	class ICVarDisplay
	{
	public:
		virtual ~ICVarDisplay() = default;

		virtual void display(const CVarValue& initial, CVarValue& current,
							 std::string_view name, std::string_view description,
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

		virtual void saveFile(std::string_view filename) = 0;
		virtual void saveFile(FSNavigator& navigator) = 0;
		virtual void loadFile(std::string_view filename) = 0;
		virtual void loadFile(FSNavigator& navigator) = 0;

		virtual void debugPrint() = 0;

		virtual void renderIGUIDisplay(ICVarDisplay& display) = 0;


		virtual CVarInfo* createFloatCVar(std::string_view name,
										  double initial = 0.0, std::string_view description = "",
										  CVarFlags flags = CVarFlagBits::DefaultArg,
										  double min = std::numeric_limits<double>::lowest(),
										  double max = std::numeric_limits<double>::max()) = 0;

		virtual CVarInfo* createIntCVar(std::string_view name,
										int64_t initial = 0, std::string_view description = "",
										CVarFlags flags = CVarFlagBits::DefaultArg,
										int64_t min = std::numeric_limits<int64_t>::lowest(),
										int64_t max = std::numeric_limits<int64_t>::max()) = 0;

		virtual CVarInfo* createBoolCVar(std::string_view name,
										 bool initial = false, std::string_view description = "",
										 CVarFlags flags = CVarFlagBits::DefaultArg) = 0;

		virtual CVarInfo* createStringCVar(std::string_view name,
										   std::string_view initial = "", std::string_view description = "",
										   CVarFlags flags = CVarFlagBits::DefaultArg) = 0;

		virtual CVarInfo* createVec2CVar(std::string_view name,
										 Vec2 initial = Vec2{}, std::string_view description = "",
										 CVarFlags flags = CVarFlagBits::DefaultArg,
										 double min = std::numeric_limits<double>::lowest(),
										 double max = std::numeric_limits<double>::max()) = 0;

		virtual CVarInfo* createVec3CVar(std::string_view name,
										 Vec3 initial = Vec3{}, std::string_view description = "",
										 CVarFlags flags = CVarFlagBits::DefaultArg,
										 double min = std::numeric_limits<double>::lowest(),
										 double max = std::numeric_limits<double>::max()) = 0;

		virtual CVarInfo* createVec4CVar(std::string_view name,
										 Vec4 initial = Vec4{}, std::string_view description = "",
										 CVarFlags flags = CVarFlagBits::DefaultArg,
										 double min = std::numeric_limits<double>::lowest(),
										 double max = std::numeric_limits<double>::max()) = 0;



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
		AutoCVar_Float(std::string_view name = "", double initial = 0.f,
					   std::string_view description = "", CVarFlags flags = CVarFlagBits::DefaultArg,
					   double min = std::numeric_limits<double>::min(), double max = std::numeric_limits<double>::max());
		double get() const;
		double* getPtr() const;
		float getFloat() const;
		float* getFloatPtr() const;
		void set(double val) const;
	};

	struct AutoCVar_Int : AutoCVar<int64_t>
	{
		AutoCVar_Int(std::string_view name = "", int64_t initial = 0,
					 std::string_view description = "", CVarFlags flags = CVarFlagBits::DefaultArg,
					 int64_t min = std::numeric_limits<int64_t>::min(),
					 int64_t max = std::numeric_limits<int64_t>::max());

		int64_t get() const;
		int64_t* getPtr() const;
		void set(int64_t val) const;
	};

	struct AutoCVar_Bool : AutoCVar<bool>
	{
		AutoCVar_Bool(std::string_view name = "", bool initial = false, std::string_view description = "", CVarFlags flags = CVarFlagBits::DefaultArg);

		bool get() const;
		bool* getPtr() const;
		void set(bool val) const;
	};

	struct AutoCVar_String : AutoCVar<std::string>
	{
		AutoCVar_String(std::string_view name = "", std::string_view initial = "", std::string_view description = "", CVarFlags flags = CVarFlagBits::DefaultArg);

		std::string_view get() const;
		void set(std::string_view val) const;
	};

	struct AutoCVar_Vec2 : AutoCVar<CVarSystem::Vec2>
	{
		AutoCVar_Vec2(std::string_view name = "", CVarSystem::Vec2 initial = CVarSystem::Vec2{ 0.,0. }, std::string_view description = "", CVarFlags flags = CVarFlagBits::DefaultArg,
					  double min = std::numeric_limits<double>::min(), double max = std::numeric_limits<double>::max());

		CVarSystem::Vec2 get() const;
		CVarSystem::Vec2* getPtr() const;
		void set(CVarSystem::Vec2 val) const;
	};

	struct AutoCVar_Vec3 : AutoCVar<CVarSystem::Vec3>
	{
		AutoCVar_Vec3(std::string_view name = "", const CVarSystem::Vec3& initial = CVarSystem::Vec3{ 0.,0.,0. }, std::string_view description = "", CVarFlags flags = CVarFlagBits::DefaultArg,
					  double min = std::numeric_limits<double>::min(), double max = std::numeric_limits<double>::max());

		CVarSystem::Vec3 get() const;
		CVarSystem::Vec3* getPtr() const;
		void set(CVarSystem::Vec3 val) const;
	};

	struct AutoCVar_Vec4 : AutoCVar<CVarSystem::Vec4>
	{
		AutoCVar_Vec4(std::string_view name = "", const CVarSystem::Vec4& initial = CVarSystem::Vec4{ 0.,0.,0. ,0. }, std::string_view description = "", CVarFlags flags = CVarFlagBits::DefaultArg,
					  double min = std::numeric_limits<double>::min(), double max = std::numeric_limits<double>::max());

		CVarSystem::Vec4 get() const;
		CVarSystem::Vec4* getPtr() const;
		void set(CVarSystem::Vec4 val) const;
	};

} // namespace utl