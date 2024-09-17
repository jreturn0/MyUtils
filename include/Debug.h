#pragma once
#include <iostream>

#include "Logger.h"



namespace Debug
{









	inline void Init()
	{
		Logger& logger = Logger::Instance();
		logger.waitForReady();
	}

	namespace
	{
		template <typename... Args>
		void LogMessage(Log::Type logType, Log& fmt, Args&&... args)
		{
			static Logger& logger = Logger::Instance();
			fmt.Apply(std::forward<Args>(args)...);
			fmt.type = logType;
			logger.addLog(fmt);
		}

		
	}

	inline static void Flush ()
	{
		static Logger& logger = Logger::Instance();
		logger.flush();
	}

	template <typename... Args>
	void Info(Log fmt, Args&&... args)
	{
		LogMessage(Log::Type::Info, fmt, std::forward<Args>(args)...);
	}
	//template <typename... Args>
	//void Out(Log fmt, Args&&... args)
	//{
	//	fmt.Apply(std::forward<Args>(args)...);
	//	fmt.type = Log::Type::Info;
	//	PrintOut(fmt, std::cout);
	//}

	template <typename... Args>
	void Warning(Log fmt, Args&&... args)
	{
		LogMessage(Log::Type::Warning, fmt, std::forward<Args>(args)...);
	}

	//template <typename... Args>
	//void Trace(Log fmt, Args&&... args)
	//{
	//	LogMessage(Log::Type::TRACE, fmt, std::forward<Args>(args)...);
	//}

	template <typename... Args>
	void Error(Log fmt, Args&&... args)
	{
		LogMessage(Log::Type::Error, fmt, std::forward<Args>(args)...);
	}

	template <typename... Args>
	void FatalError(Log fmt, Args&&... args)
	{
		LogMessage(Log::Type::FatalError, fmt, std::forward<Args>(args)...);
	}
	template <typename... Args>
	void Exception(Log fmt, Args&&... args)
	{
		LogMessage(Log::Type::Exception, fmt, std::forward<Args>(args)...);
	}

	template <typename... Args>
	void Throw (Log fmt, Args&&... args)
	{
		fmt.Apply(std::forward<Args>(args)...);
		throw std::runtime_error(fmt.message);
	}



	template <typename... Args>
	void Assert(bool condition, Log fmt, Args&&... args)
	{
		if (!condition) {
			fmt.Apply(std::forward<Args>(args)...);
			fmt.type = Log::Type::FatalError;
			fmt.message = fmt.message + "\nAssertion failed at " + fmt.source.file_name() + ":" + std::to_string(fmt.source.line()) + " in " + fmt.source.function_name();
			//PrintOut(fmt, std::cerr); // Print to standard error
			LogMessage(Log::Type::Assert, fmt, std::forward<Args>(args)...);
			std::abort(); // Terminate the program
		}
	}
	template <typename... Args>
	void AssertThrow(bool condition, Log fmt, Args&&... args)
	{
		if (!condition) {
			fmt.Apply(std::forward<Args>(args)...);
			fmt.type = Log::Type::FatalError;
			fmt.message = fmt.message + "\nAssertion failed at " + fmt.source.file_name() + ":" + std::to_string(fmt.source.line()) + " in " + fmt.source.function_name();
			PrintOut(fmt, std::cerr); // Print to standard error
			//LogMessage(Log::Type::Assert, fmt, std::forward<Args>(args)...);
			throw std::runtime_error(fmt.message);

		}
	}



}