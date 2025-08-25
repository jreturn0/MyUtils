#pragma once

#include <string_view>
#include <array>
#include <variant>
#include <stdint.h>
#include "StringHash.h"
#include "BitFlags.h"
#include <limits>
#include <string>
#include <memory>

class FSNavigator;

namespace utl {




	struct CVarParameter;





















	//class ICVarDisplay
	//{
	//public:
	//	 ~ICVarDisplay() = default;

	//	 void display(const CVarValue& defaultValue, CVarValue& current,
	//		std::string_view name, std::string_view description,
	//		CVarFlags flags);
	//};

		enum class CVarFlagBits : uint16_t {
			none,
			archive = 1 << 0,
			readonly = 1 << 1,
			hidden = 1 << 2,
			cheat = 1 << 3,
			notify = 1 << 4,
		};
		using CVarFlags = utl::BitFlags<CVarFlagBits>;


	class CVarSystem
	{
	public:

		~CVarSystem() = default;

		// Singleton access
		static CVarSystem* getInstance();

		virtual CVarParameter* getCVarParameter(StringHash name) = 0;


	   // Create a new CVar of the specified type
		virtual CVarParameter* createFloatCVar(std::string_view name, double defaultValue, CVarFlags flags, std::string_view description = "") = 0;
		virtual CVarParameter* createIntCVar(std::string_view name, int64_t defaultValue, CVarFlags flags, std::string_view description = "") = 0;
		virtual CVarParameter* createBoolCVar(std::string_view name, bool defaultValue, CVarFlags flags, std::string_view description = "") = 0;
		virtual CVarParameter* createStringCVar(std::string_view name, std::string_view defaultValue, CVarFlags flags, std::string_view description = "") = 0;


		// Set value of an existing CVar
		virtual void setFloatCVar(StringHash hash, double values) = 0;
		virtual void setIntCVar(StringHash hash, int64_t values) = 0;
		virtual void setBoolCVar(StringHash hash, bool values) = 0;
		virtual void setStringCVar(StringHash hash, std::string_view values) = 0;




		// Get value of an existing CVar
		virtual double getFloatCVar(StringHash hash) = 0;
		virtual int64_t     getIntCVar(StringHash hash)=0;
		virtual bool     getBoolCVar(StringHash hash)=0;
		virtual std::string_view getStringCVar(StringHash hash)=0;


		virtual void debugPrintCVars() = 0;
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
		AutoCVar_Float(std::string_view name, double defaultValue,
			CVarFlags flags = {}, std::string_view description = {});
		double get() const;
		double* getPtr() const;
		float getFloat();
		float* getFloatPtr();
		void set(double val) const;
	};

	struct AutoCVar_Int : AutoCVar<int64_t>
	{
		AutoCVar_Int(std::string_view name, int64_t defaultValue,
			CVarFlags flags = {}, std::string_view description = {});
		int64_t get() const;
		int64_t* getPtr() const;
		void set(int64_t val) const;
	};

	struct AutoCVar_Bool : AutoCVar<bool>
	{
		AutoCVar_Bool(std::string_view name, bool defaultValue,
			CVarFlags flags = {}, std::string_view description = {});
		bool get() const;
		bool* getPtr() const;
		void set(bool val) const;
	};

	struct AutoCVar_String : AutoCVar<std::string>
	{
		AutoCVar_String(std::string_view name, std::string& defaultValue,
			CVarFlags flags = {}, std::string_view description = {});
		std::string_view get() const;
		std::string getCopy() const;
		void set(std::string_view val) const;
		void set(const std::string& val) const;
	};


} // namespace utl