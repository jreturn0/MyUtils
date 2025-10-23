#include "AnsiCodes.h"
#include "Logger.h"
#include <fstream>
#include <iostream>
#include <sstream>

namespace {
#ifdef LOGGER_USE_ANSI
    constexpr auto g_traceTag = "[\x1b[2m\x1b[30mTRACE\x1b[0m]";
    constexpr auto g_infoTag = "[\x1b[32mINFO\x1b[0m]";
    constexpr auto g_warningTag = "[\x1b[33mWARNING\x1b[0m]";
    constexpr auto g_errorTag = "[\x1b[31mERROR\x1b[0m]";
    constexpr auto g_fatalErrorTag = "[\x1b[1m\x1b[37m\x1b[41mFATAL ERROR\x1b[0m]";
    constexpr auto g_exceptionTag = "[\x1b[35mEXCEPTION\x1b[0m]";
    constexpr auto g_assertTag = "[\x1b[36mASSERT\x1b[0m]";
#else
    constexpr auto g_traceTag = "[TRACE]";
    constexpr auto g_infoTag = "[INFO]";
    constexpr auto g_warningTag = "[WARNING]";
    constexpr auto g_errorTag = "[ERROR]";
    constexpr auto g_fatalErrorTag = "[FATAL ERROR]";
    constexpr auto g_exceptionTag = "[EXCEPTION]";
    constexpr auto g_assertTag = "[ASSERT]";
#endif





    static constexpr bool operator ==(const std::source_location& lhs, const std::source_location& rhs)
    {
        return (lhs.file_name() == rhs.file_name()) && (lhs.line() == rhs.line()) && (lhs.column() == rhs.column());
    }

}


Debug::Logger::Logger() : logMask(g_allLogTypes), messageQueue(), tempQueue(), readyFuture(readyPromise.get_future().share())
{
    thread = std::thread(&Logger::runAsync, this);
}
Debug::Logger::~Logger()
{
    flush();
    running.store(false);
    cv.notify_one();
    thread.join();
}

static void PrintOut(Debug::Log& log, std::ostream& stream)
{
    std::ostringstream oss;
    oss << StreamLogType(log.type);
    oss << log.source.file_name() << ":" << log.source.line() << "\n"
        << log.message << "\n";
    stream << oss.str();
}




static void formatLog(std::vector<char>& buffer, const Debug::Log& log) {
    std::format_to(std::back_inserter(buffer), "{} {}({},{}) - {}\n",
        StreamLogType(log.type),
        log.source.file_name(), log.source.line(), log.source.column(),
        log.message);
}


void Debug::Logger::runAsync()
{
    std::ostringstream oss;
    std::fstream file{ "log.txt", std::ios::out | std::ios::app };
    std::source_location prevSource{}; // Temp unused
    readyPromise.set_value();

    std::vector<char> buffer;
    buffer.reserve(1024);
    while (running) {
        {
            std::unique_lock lock(mtx);
            cv.wait(lock, [this] {
                return !messageQueue.isEmpty() || !running;
                });
            messageQueue.swap(tempQueue);
            lock.unlock();
        }
        messageQueue.lock();
        Log log;
        while (tempQueue.dequeueUnsafe(log)) {
            buffer.clear();
            if (prevSource == log.source)
                std::format_to(std::back_inserter(buffer), "|> {}\n", log.message);
            else
                std::format_to(std::back_inserter(buffer), "{} {}({},{}):\n|> {}\n",
                    StreamLogType(log.type),
                    log.source.file_name(), log.source.line(), log.source.column(),
                    log.message);
            oss.write(buffer.data(), buffer.size());
            prevSource = log.source;
            pendingMessages.fetch_sub(1);
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
        pendingMessages.fetch_add(1);

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
    stream << std::format("{}[{}:{}]\n", StreamLogType(log.type), log.source.file_name(), log.source.line());
}



constexpr std::string_view Debug::StreamLogType(const Debug::Log::Type type) noexcept
{

    switch (type) {
    case Debug::Log::Type::None:
        return "";
    case Debug::Log::Type::Trace:
        return g_traceTag;
    case Debug::Log::Type::Info:
        return  g_infoTag;
    case Debug::Log::Type::Warning:
        return  g_warningTag;
    case Debug::Log::Type::Error:
        return  g_errorTag;
    case Debug::Log::Type::FatalError:
        return  g_fatalErrorTag;
    case Debug::Log::Type::Exception:
        return  g_exceptionTag;
    case Log::Type::Assert:
        return  g_assertTag;
    default:
        return "";
    }
}


void Debug::Logger::waitForReady() const
{
    readyFuture.wait();
}

void Debug::Logger::setLogMask(Log::TypeFlags mask) { logMask = mask; }
