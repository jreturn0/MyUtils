#pragma once
#include <string>
#include <source_location>
#include <format>
#include <stdint.h>
namespace Debug
{
	struct Log
	{

		enum class Type : uint8_t
		{
			None = 0,
			Trace,
			Info,
			Warning,
			Error,
			FatalError,
			Exception,
			Assert
		} type;
		std::string message;
		std::source_location source;

		template <typename... Args>
		void Apply(Args &&...args);

		Log(const char *fmt, Type t = Type::Info, const std::source_location &l = std::source_location::current());
		Log(std::string fmt, Type t = Type::Info, const std::source_location &l = std::source_location::current());
		Log() = default;
	};
}
template <typename... Args>
inline void Debug::Log::Apply(Args &&...args)
{
	message = std::vformat(message, std::make_format_args(args...));
}