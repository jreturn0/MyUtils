#pragma once
#include "Log.h"
#include <condition_variable>
#include <future>
#include <mutex>
#include <ostream>
#include <queue>
#include <thread>

#include "FixedQueue.h"


namespace	Debug {

    //How many logs could a logger log if a logger could log logs
    class Logger
    {
    public:
        Logger(const Logger&) = delete;
        Logger(Logger&&) = delete;
        Logger& operator=(const Logger&) = delete;
        Logger& operator=(Logger&&) = delete;

        static Logger& Instance()
        {
            static Logger logger;
            return logger;
        }


        bool isEmpty();
        void addLog(const Log& log);
        void dump();
        void flush();
        void waitForReady();
        void waitForReady() const;
        void setLogMask(Log::TypeFlags mask);
    private:
        Log::TypeFlags logMask;
        std::thread thread;
        std::atomic_bool running = true;
        std::mutex mtx;
        utl::FixedQueue<Log,1024> messageQueue;
        utl::FixedQueue<Log,1024> tempQueue;
        std::condition_variable cv;
        std::atomic_size_t pendingMessages = 0;
        std::promise<void> readyPromise;
        std::shared_future<void> readyFuture;

        Logger();
        ~Logger();

        void runAsync();
    };
    void PrintOut(Debug::Log& log, std::ostream& stream);
    constexpr std::string_view StreamLogType(Debug::Log::Type type) noexcept;
};
