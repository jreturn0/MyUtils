#pragma once
#include "Log.h"
#include <thread>
#include <queue>
#include <condition_variable>
#include <mutex>
#include <future>
#include <ostream>

#include "FixedQueue.h"


namespace	Debug {

	//How many logs could a logger log if a logger could log logs
	class Logger
	{
	private:

		std::thread thread;
		std::atomic_bool running = true;
		std::mutex mtx;
		FixedQueue<Log,1024> messageQueue;
		FixedQueue<Log,1024> tempQueue;
		std::condition_variable cv;
		std::atomic<int> pendingMessages = 0;
		std::promise<void> readyPromise;
		std::shared_future<void> readyFuture;

		Logger();
		~Logger();

		void runAsync();

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


	};
	void PrintOut(Debug::Log& log, std::ostream& stream);
	void StreamLogType(std::basic_ostream<char>& stream, Debug::Log::Type type);
};
