#pragma once
#include "BitFlags.h"
#include <format>
#include <source_location>
#include <stdint.h>
#include <string>
namespace Debug
{
    struct Log
    {

        enum class Type 
        {
            None = 0 << 0,
            Trace = 1 << 0,
            Info = 1 << 1,
            Warning = 1 << 2,
            Error= 1 << 3,
            FatalError = 1 << 4,
            Exception= 1 << 5,
            Assert = 1 << 6
        } type{ Type::None };
        using TypeFlags = utl::BitFlags<Type>;
        std::string message;
        std::source_location source;

        template <typename... Args>
        void Apply(Args &&...args);

        Log(const char* fmt, Type t = Type::Info, const std::source_location& l = std::source_location::current());
        Log(std::string fmt, Type t = Type::Info, const std::source_location& l = std::source_location::current());
        Log() = default;
    };

    constexpr Debug::Log::TypeFlags g_allLogTypes = Debug::Log::TypeFlags(Debug::Log::Type::Trace) |
        Debug::Log::Type::Info |
        Debug::Log::Type::Warning |
        Debug::Log::Type::Error |
        Debug::Log::Type::FatalError |
        Debug::Log::Type::Exception |
        Debug::Log::Type::Assert;
}
template <typename... Args>
inline void Debug::Log::Apply(Args &&...args)
{
    message = std::vformat(message, std::make_format_args(args...));
}