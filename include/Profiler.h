#pragma once
#include <unordered_map>
struct TimeStats
{
	long long frameTime{0};
	long long minFrameTime{std::numeric_limits<long long>::max()};
	long long maxFrameTime{0};
	long long avgFrameTime{0};
	long long rollingAvgFrameTime{ 0 };

	void update(const long long time)
	{
		frameTime = time;
		minFrameTime = std::min(minFrameTime, time);
		maxFrameTime = std::max(maxFrameTime, time);
		avgFrameTime = (avgFrameTime + time) / 2;
	}
};

class Profiler
{
private:

public:
	
};
