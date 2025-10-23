//#pragma once
//#include <algorithm>
//#include <unordered_map>
//#include <chrono>
//#include <iostream>
//#include <numeric>
//
//
//namespace utl {
//	inline std::string FormatWithCommas(const long long value)
//	{
//		std::ostringstream oss;
//		oss.imbue(std::locale("en_US.UTF-8"));
//		oss << value;
//		return oss.str();
//	}
//	struct TimeStats
//	{
//
//
//		int64_t counts{ 0ull };
//		std::chrono::nanoseconds totalDuration{ 0ull };
//		std::chrono::nanoseconds minDuration{ std::chrono::nanoseconds::max() };
//		std::chrono::nanoseconds maxDuration{ std::chrono::nanoseconds::min() };
//
//		[[nodiscard]] std::chrono::nanoseconds averageDuration() const { return counts > 0 ? std::chrono::nanoseconds(totalDuration.count() / counts) : std::chrono::nanoseconds{ 0 }; }
//
//		void update(const std::chrono::nanoseconds duration)
//		{
//			totalDuration += duration;
//			++counts;
//			minDuration = std::min(minDuration, duration);
//			maxDuration = std::max(maxDuration, duration);
//		}
//
//		void reset()
//		{
//			counts = 0ll;
//			totalDuration = std::chrono::nanoseconds::min();
//			minDuration = std::chrono::nanoseconds::max();
//			maxDuration = std::chrono::nanoseconds::min();
//		}
//
//		void print() const
//		{
//			std::cout << "\tTotal: " << FormatWithCommas(totalDuration.count()) << "ns\n";
//			std::cout << "\tAverage: " << FormatWithCommas(averageDuration().count()) << "ns\n";
//			std::cout << "\tMin: " << FormatWithCommas(minDuration.count()) << "ns\n";
//			std::cout << "\tMax: " << FormatWithCommas(maxDuration.count()) << "ns\n";
//			std::cout << "\tCount: " << FormatWithCommas(counts) << "\n";
//		}
//		void print(std::ostream& os) const
//		{
//			os << "\tTotal: " << FormatWithCommas(totalDuration.count()) << "ns\n";
//			os << "\tAverage: " << FormatWithCommas(averageDuration().count()) << "ns\n";
//			os << "\tMin: " << FormatWithCommas(minDuration.count()) << "ns\n";
//			os << "\tMax: " << FormatWithCommas(maxDuration.count()) << "ns\n";
//			os << "\tCount: " << FormatWithCommas(counts) << "\n";
//		}
//
//		std::string toString() const
//		{
//			std::string str;
//			str += "\tTotal: " + FormatWithCommas(totalDuration.count()) + "ns\n";
//			str += "\tAverage: " + FormatWithCommas(averageDuration().count()) + "ns\n";
//			str += "\tMin: " + FormatWithCommas(minDuration.count()) + "ns\n";
//			str += "\tMax: " + FormatWithCommas(maxDuration.count()) + "ns\n";
//			str += "\tCount: " + FormatWithCommas(counts) + "\n";
//			return str;
//		}
//	};
//
//
//	struct Timer
//	{
//		TimeStats* stats;
//		std::chrono::time_point<std::chrono::high_resolution_clock> start;
//
//		using Duration = std::chrono::duration<unsigned long long int, std::nano>;
//
//		Timer() : stats{ nullptr }, start(std::chrono::high_resolution_clock::now()) {}
//		explicit Timer(TimeStats& stats) : stats{ &stats }, start(std::chrono::high_resolution_clock::now()) {}
//		~Timer()
//		{
//			if (stats) {
//				const auto end = std::chrono::high_resolution_clock::now();
//				const auto duration = end - start;
//				stats->update(duration);
//			}
//		};
//
//		// Get the duration in milliseconds
//		[[nodiscard]] std::chrono::nanoseconds getDuration() const
//		{
//			const auto end = std::chrono::high_resolution_clock::now();
//			return end - start;
//		}
//
//	};
//	class Profiler
//	{
//	private:
//		std::unordered_map<std::string, TimeStats> stats;
//		std::chrono::time_point<std::chrono::high_resolution_clock> start;
//		std::string currentName;
//
//	public:
//
//	};
//}
//
//
//
