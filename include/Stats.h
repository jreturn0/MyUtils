#pragma once
#include <algorithm>

namespace utl {
    struct TimeStats
    {
        int64_t frameTime{ 0 };
        int64_t minFrameTime{ std::numeric_limits<int64_t>::max()};
        int64_t maxFrameTime{ std::numeric_limits<int64_t>::min() };
        int64_t avgFrameTime{ 0 };
        void update(const long long time)
        {
            frameTime = time;
            minFrameTime = std::min(minFrameTime, time);
            maxFrameTime = std::max(maxFrameTime, time);
            avgFrameTime = (avgFrameTime + time) / 2;
        }
    };

}