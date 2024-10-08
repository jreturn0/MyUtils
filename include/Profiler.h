#pragma once
#include <algorithm>
#include <unordered_map>
#include <chrono>
#include <numeric>


namespace utl {
	struct TimeStats
	{


		int64_t counts{ 0ull };
		std::chrono::nanoseconds totalDuration{ 0ull };
		std::chrono::nanoseconds minDuration{ std::chrono::nanoseconds::max() };
		std::chrono::nanoseconds maxDuration{ std::chrono::nanoseconds::min() };

		[[nodiscard]] std::chrono::nanoseconds averageDuration() const { return counts > 0 ? std::chrono::nanoseconds(totalDuration.count() / counts) : std::chrono::nanoseconds{ 0 }; }

		void update(const std::chrono::nanoseconds duration)
		{
			totalDuration += duration;
			++counts;
			minDuration = std::min(minDuration, duration);
			maxDuration = std::max(maxDuration, duration);
		}

		void reset()
		{
			counts = 0ll;
			totalDuration = std::chrono::nanoseconds::min();
			minDuration = std::chrono::nanoseconds::max();
			maxDuration = std::chrono::nanoseconds::min();
		}
	};


	struct Timer
	{
		TimeStats* stats;
		std::chrono::time_point<std::chrono::high_resolution_clock> start;

		using Duration = std::chrono::duration<unsigned long long int, std::nano>;

		Timer() : stats{ nullptr }, start(std::chrono::high_resolution_clock::now()) {}
		explicit Timer(TimeStats& stats) : stats{ &stats }, start(std::chrono::high_resolution_clock::now()) {}
		~Timer()
		{
			if (stats) {
				const auto end = std::chrono::high_resolution_clock::now();
				const auto duration = end - start;
				stats->update(duration);
			}
		};

		// Get the duration in milliseconds
		[[nodiscard]] std::chrono::nanoseconds getDuration() const
		{
			const auto end = std::chrono::high_resolution_clock::now();
			return end - start;
		}

	};
	class Profiler
	{
	private:
		std::unordered_map<std::string, TimeStats> stats;
		std::chrono::time_point<std::chrono::high_resolution_clock> start;
		std::string currentName;

	public:

	};
}



