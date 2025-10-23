#pragma once
#include <iostream>

#include "Logger.h"



namespace Debug
{
    inline void Init(Log::TypeFlags logMask = g_allLogTypes)
    {
        Logger& logger = Logger::Instance();
        logger.setLogMask(logMask);
        logger.waitForReady();
    }


    template <typename... Args>
    inline void LogMessage(Log::Type logType, Log& fmt, Args&&... args)
    {
        static Logger& logger = Logger::Instance();
        fmt.Apply(std::forward<Args>(args)...);
        fmt.type = logType;
        logger.addLog(fmt);
    }

    inline void Flush()
    {
        static Logger& logger = Logger::Instance();
        logger.flush();
    }

    template <typename... Args>
    inline void Info(Log fmt, Args&&... args)
    {
        LogMessage(Log::Type::Info, fmt, std::forward<Args>(args)...);
    }

    template <typename... Args>
    inline void Warning(Log fmt, Args&&... args)
    {
        LogMessage(Log::Type::Warning, fmt, std::forward<Args>(args)...);
    }

    template <typename... Args>
    inline void Error(Log fmt, Args&&... args)
    {
        LogMessage(Log::Type::Error, fmt, std::forward<Args>(args)...);
    }

    template <typename... Args>
    inline void FatalError(Log fmt, Args&&... args)
    {
        LogMessage(Log::Type::FatalError, fmt, std::forward<Args>(args)...);
    }
    template <typename... Args>
    inline void Exception(Log fmt, Args&&... args)
    {
        LogMessage(Log::Type::Exception, fmt, std::forward<Args>(args)...);
    }

    template <typename... Args>
    inline void Throw(Log fmt, Args&&... args)
    {
        fmt.Apply(std::forward<Args>(args)...);
        throw std::runtime_error(fmt.message);
    }

    template <typename... Args>
    inline void Assert(bool condition, Log fmt, Args&&... args)
    {
        if (!condition) {
            fmt.Apply(std::forward<Args>(args)...);
            fmt.type = Log::Type::FatalError;
            fmt.message = std::format("\nAssertion failed at {}:{} in {}\n", fmt.source.file_name(), fmt.source.line(), fmt.source.function_name());
            LogMessage(Log::Type::Assert, fmt, std::forward<Args>(args)...);
            std::abort(); 
        }
    }
    template <typename... Args>
    inline void AssertThrow(bool condition, Log fmt, Args&&... args)
    {
        if (!condition) {
            fmt.Apply(std::forward<Args>(args)...);
            fmt.type = Log::Type::FatalError;
            fmt.message = std::format("\nAssertion failed at {}:{} in {}: {}\n",fmt.source.file_name(),fmt.source.line(),fmt.source.function_name(),fmt.message);
            PrintOut(fmt, std::cerr); 
            throw std::runtime_error(fmt.message);

        }
    }
}