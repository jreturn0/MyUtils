#include "Logger.h"
#include "AnsiCodes.h"
#include <sstream>
#include <fstream>
#include <iostream>


Debug::Logger::Logger() : messageQueue(), tempQueue(), readyFuture(readyPromise.get_future().share())
{
	thread = std::thread(&Logger::runAsync, this);
}
Debug::Logger::~Logger()
{
	flush();
	running = false;
	cv.notify_one();
	thread.join();
}

void PrintOut(Debug::Log& log, std::ostream& stream)
{
	std::ostringstream oss;
	StreamLogType(oss, log.type);
	oss << log.source.file_name() << ":" << log.source.line() << "\n"
		<< log.message << "\n";
	stream << oss.str();
}

bool operator ==(const std::source_location& lhs, const std::source_location& rhs)
{
	return lhs.file_name() == rhs.file_name() && lhs.line() == rhs.line();
}


void Debug::Logger::runAsync()
{
	std::ostringstream oss;
	std::fstream file{ "log.txt", std::ios::out | std::ios::app };
	std::source_location preSource{};
	readyPromise.set_value();
	while (running) {
		{
			std::unique_lock<std::mutex> lock(mtx);
			cv.wait(lock, [this] {
				return !messageQueue.isEmpty() || !running;
					});
			messageQueue.swap(tempQueue);
			lock.unlock();
		}
		messageQueue.lock();
		Log log;
		while (tempQueue.dequeueUnsafe(log)) {
			if (log.source != preSource) {
				preSource = log.source;
				StreamLogType(oss, log.type);
				oss << ansi::dim << log.source.file_name() << ":" << log.source.line() << ansi::reset << "\n";
			}
			oss	<< log.message << "\n";
			--pendingMessages;
		}
		messageQueue.unlock();
		if (!oss.str().empty()) {
			std::cout << oss.str();
			file << oss.str();
			oss.str("");
			oss.clear();
		}
	}

	Log log;
	messageQueue.lock();
	while (messageQueue.dequeueUnsafe(log)) {
		oss << log.source.file_name() << ":" << log.source.line() << "\n"
			<< log.message << "\n";
	}
	messageQueue.unlock();
}

bool Debug::Logger::isEmpty()
{
	std::lock_guard<std::mutex> lock(mtx);
	return messageQueue.isEmpty() && tempQueue.isEmpty() && (pendingMessages.load() == 0);
}

void Debug::Logger::addLog(const Log& log)
{
	{
		std::lock_guard<std::mutex> lock(mtx);
		messageQueue.enqueue(log);
		++pendingMessages;

	}
	cv.notify_one();
}

void Debug::Logger::dump()
{
	std::ofstream dump{ "log.txt" };

}

void Debug::Logger::flush()
{
	while (!isEmpty()) { std::this_thread::yield(); }
}

void Debug::Logger::waitForReady()
{
	readyFuture.wait();
}

void Debug::PrintOut(Debug::Log& log, std::ostream& stream)
{
	std::ostringstream oss;
	StreamLogType(oss, log.type);
	oss << log.source.file_name() << ":" << log.source.line() << "\n"
		<< log.message << "\n";
	stream << oss.str();
}

void Debug::StreamLogType(std::basic_ostream<char>& stream, const Debug::Log::Type type)
{
	switch (type) {
	case Debug::Log::Type::None:
		return;
	case Debug::Log::Type::Trace:
		stream << '[' << ansi::dim << ansi::black  << "TRACE" << ansi::reset << "] ";
		break;
	case Debug::Log::Type::Info:
		stream << '[' << ansi::green  << "INFO" << ansi::reset << "] ";
		break;
	case Debug::Log::Type::Warning:
		stream << '[' << ansi::yellow  << "WARNING" << ansi::reset << "] ";
		break;
	case Debug::Log::Type::Error:
		stream << '[' << ansi::red  << "ERROR" << ansi::reset << "] ";
		break;
	case Debug::Log::Type::FatalError:
		stream << '[' << ansi::bold << ansi::white << ansi::bg_red  << "FATAL ERROR" << ansi::reset << "] ";
		break;
	case Debug::Log::Type::Exception:
		stream << '[' << ansi::magenta  << "EXCEPTION" << ansi::reset << "] ";
		break;
	case Log::Type::Assert:
		stream << '[' << ansi::cyan  << "ASSERT" << ansi::reset << "] ";
		break;
	default:
		break;
	}
}


void Debug::Logger::waitForReady() const
{
	readyFuture.wait();
}
