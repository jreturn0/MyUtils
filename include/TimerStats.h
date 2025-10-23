#pragma once
#include <algorithm>
#include <array>
#include <chrono>
#include <cstdint>
#include <limits>
#include <numeric>
#include <thread>

#ifdef _DEBUG
#ifndef UTL_ENABLE_TIMING
#define UTL_ENABLE_TIMING 1
#endif 
#endif 



namespace utl {

    namespace details {
        template<size_t N = 60>
        struct TimerStatsRolling {
            std::array<uint64_t, N> values{};
            size_t index{ 0 };
            size_t filledCount{ 0 }; // number of valid samples

            TimerStatsRolling() {
                values.fill(0);
            }

            inline size_t count() const noexcept {
                return filledCount;
            }

            inline void reset() noexcept {
                values.fill(std::numeric_limits<uint64_t>::max());
                index = 0;
                filledCount = 0;
            }

            inline void update(uint64_t durationNs) noexcept {
                values[index] = durationNs;
                index = (index + 1) % N;
                if (filledCount < N) ++filledCount;
            }

            inline std::pair<uint64_t, uint64_t> minmaxNs() const noexcept {
                if (filledCount == 0) return { 0, 0 };
                auto first = values.begin();
                auto last = first + filledCount;
                auto [minIt, maxIt] = std::ranges::minmax_element(first, last);
                return { *minIt, *maxIt };
            }

            inline uint64_t minNs() const noexcept {
                if (filledCount == 0) return 0;

                return *std::ranges::min_element(values.begin(), values.begin() + filledCount);

            }

            inline uint64_t maxNs() const noexcept {
                if (filledCount == 0) return 0;

                return *std::ranges::max_element(values.begin(), values.begin() + filledCount);

            }

            inline uint64_t totalNs() const noexcept {
                return std::accumulate(values.begin(), values.begin() + filledCount, 0ull);
            }

            inline double avgNs() const noexcept {
                return filledCount ? static_cast<double>(totalNs()) / filledCount : 0.0;
            }

            inline double avgMs() const noexcept {
                return avgNs() / 1'000'000.0;
            }
        };


        // TimerStats using a batch count and sum for average calculation
        template<size_t batchSize = 20>
        struct TimerStatsBatch {
            uint64_t count{ 0ull };
            uint64_t totalNs{ 0ull };
            uint64_t minNs{ std::numeric_limits<uint64_t>::max() };
            uint64_t maxNs{ 0ull };
            uint64_t batchSumNs{ 0ull };
            uint64_t lastBatchAvgNs{ 0ull };

            inline void reset() noexcept {
                count = 0;
                totalNs = 0;
                minNs = std::numeric_limits<uint64_t>::max();
                maxNs = 0;
                batchSumNs = 0;
            }
            inline void update(const uint64_t durationNs) noexcept {
                ++count;
                totalNs += durationNs;
                minNs = std::min(minNs, durationNs);
                maxNs = std::max(maxNs, durationNs);
                batchSumNs += durationNs;
                if ((count % batchSize) == 0) {
                    lastBatchAvgNs = batchSumNs / batchSize;
                    batchSumNs = 0;
                }
            }
            inline uint64_t avgNs() const noexcept { return lastBatchAvgNs; }
            inline double avgMs() const noexcept { return avgNs() / 1'000'000.0; }
        };





        struct TimerStatsEWMA {
            double alpha{ 0.985 }; // smoothing factor for EWMA
            uint64_t count{ 0ull };
            uint64_t totalNs{ 0ull };
            uint64_t minNs{ std::numeric_limits<uint64_t>::max() };
            uint64_t maxNs{ 0ull };
            double avgNs{ 1.0 };
            inline void reset() noexcept {
                count = 0;
                totalNs = 0;
                minNs = std::numeric_limits<uint64_t>::max();
                maxNs = 0;
                avgNs = 1.0;
            }
            inline void update(const uint64_t durationNs) noexcept {
                ++count;
                totalNs += durationNs;
                minNs = std::min(minNs, durationNs);
                maxNs = std::max(maxNs, durationNs);
                avgNs = alpha * avgNs + (1.0 - alpha) * static_cast<double>(durationNs);
            }

            inline double   avgMs()    const noexcept { return avgNs / 1'000'000.0; }
            inline double   totalMs()  const noexcept { return static_cast<double>(totalNs) / 1'000'000.0; }
            inline double   minMs()    const noexcept { return static_cast<double>(minNs) / 1'000'000.0; }
            inline double   maxMs()    const noexcept { return static_cast<double>(maxNs) / 1'000'000.0; }
        };



        // Timer statistics, single-threaded use
        struct TimerStats {
            uint64_t count{ 0ull };
            uint64_t totalNs{ 0ull };
            uint64_t minNs{ std::numeric_limits<uint64_t>::max() };
            uint64_t maxNs{ 0ull };


            inline void reset() noexcept {
                count = 0;
                totalNs = 0;
                minNs = std::numeric_limits<uint64_t>::max();
                maxNs = 0;
            }

            inline void update(const uint64_t durationNs) noexcept {
                ++count;
                totalNs += durationNs;
                minNs = std::min(minNs, durationNs);
                maxNs = std::max(maxNs, durationNs);
            }

            inline uint64_t avgNs()    const noexcept { return (count == 0) ? 0 : totalNs / count; }
            inline double   avgMs()    const noexcept { return static_cast<double>(avgNs()) / 1'000'000.0; }

        };
#ifdef UTL_ENABLE_TIMING
        template<class Stats = TimerStats>
        class ScopeTimer final {
        public:
            ScopeTimer(Stats& stats) noexcept
                : m_stats(stats), m_startTime(Clock::now())
            {
            }

            ~ScopeTimer() noexcept
            {
                const auto endTime = Clock::now();
                const auto duration = std::chrono::duration_cast<Duration>(endTime - m_startTime).count();
                m_stats.update(static_cast<uint64_t>(duration));
            }



            ScopeTimer(const ScopeTimer&) = delete;
            ScopeTimer& operator=(const ScopeTimer&) = delete;

        private:
            using Clock = std::chrono::steady_clock;
            using TimePoint = Clock::time_point;
            using Duration = std::chrono::nanoseconds;
            Stats& m_stats;
            TimePoint m_startTime{};
        };
#else
        template<class Stats = TimerStats>
        class ScopeTimer final {
        public:
            ScopeTimer(TimerStats&) noexcept {}
        };
#endif 
    } // namespace details

    using TimerStats = details::TimerStatsEWMA;
    using ScopeTimer = details::ScopeTimer<TimerStats>;



}

#define UTL_CONCAT_INNER(a,b) a##b
#define UTL_CONCAT(a,b) UTL_CONCAT_INNER(a,b)
#define UTL_SCOPED_TIMER(statsRef) ::utl::ScopeTimer UTL_CONCAT(_scopedTimer_, __LINE__){ statsRef }


namespace utl::exp {
    class Clock {
    public:
        explicit Clock(uint64_t variableFpsCap, uint64_t fixedFpsCap, uint64_t maxAccumulated = 4) :
            m_targetFps(variableFpsCap),
            m_targetFrameDuration(Duration(m_targetFps != 0 ? 1'000'000'000 / m_targetFps : 0)), //TODO: division rounding?
            m_fixedTargetFps(fixedFpsCap),
            m_fixedTargetFrameDuration(Duration(m_fixedTargetFps != 0 ? 1'000'000'000 / m_fixedTargetFps : 0)),
            m_maxAccumulatedTime(Duration(m_fixedTargetFrameDuration* maxAccumulated)),
            m_accumulatedTime(Duration::zero()),
            m_fixedFrame(false),
            m_lastFrameTime(),
            m_nextFrameTime(),
            m_elapsedTime(Duration::zero()),
            m_deltaTime(Duration::zero())
        {

        }
        Clock(const Clock&) = delete;
        Clock(Clock&&) = delete;
        Clock& operator=(const Clock&) = delete;
        Clock& operator=(Clock&&) = delete;


        void update() noexcept {
            TimePoint now = SystemClock::now();
            // If we're behind schedule, skip sleeping entirely
            if ((now < m_nextFrameTime) && (m_targetFps > 0)) {
                auto remaining = m_nextFrameTime - now;
                if (remaining > std::chrono::microseconds(200)) {
                    auto coarseSleep = remaining - remaining / 7;
                    std::this_thread::sleep_for(coarseSleep);
                }
                while ((now = SystemClock::now()) < m_nextFrameTime) {
                    std::memory_order_relaxed;
                }
            }
            //TODO: Cap delta?
            m_deltaTime = now - m_lastFrameTime;
            m_lastFrameTime = now;
            m_nextFrameTime += m_targetFrameDuration;
            m_elapsedTime += m_deltaTime;

            m_accumulatedTime += m_deltaTime;
            if (m_accumulatedTime > m_maxAccumulatedTime) {
                m_accumulatedTime = m_maxAccumulatedTime; // Prevent death spiral
            }


        }
        bool isFixed() const noexcept {
            return m_accumulatedTime >= m_fixedTargetFrameDuration;
        }
        bool fixedUpdate() noexcept {
            if (m_accumulatedTime >= m_fixedTargetFrameDuration) {
                m_accumulatedTime -= m_fixedTargetFrameDuration;
                return true;
            }
            return false;
        }



        // Getters
        inline bool isFixedFrame() const noexcept { return m_fixedFrame; }
        inline double getDelta() const noexcept { return std::chrono::duration<double>(m_deltaTime).count(); }
        inline double getElapsed() const noexcept { return std::chrono::duration<double>(m_elapsedTime).count(); }
        inline double getFixedStep() const noexcept { return std::chrono::duration<double>(m_fixedTargetFrameDuration).count(); }
        inline double getAccumulatedTime() const noexcept { return std::chrono::duration<double>(m_accumulatedTime).count(); }


    private:
        using SystemClock = std::chrono::high_resolution_clock;
        using TimePoint = SystemClock::time_point;
        using Duration = std::chrono::nanoseconds;

        // Variable targets
        uint64_t m_targetFps;
        Duration m_targetFrameDuration;

        // Fixed targets
        uint64_t m_fixedTargetFps;
        Duration m_fixedTargetFrameDuration;

        // Accumulation
        Duration m_maxAccumulatedTime;
        Duration m_accumulatedTime;
        bool m_fixedFrame;

        // Timing
        TimePoint m_lastFrameTime;
        TimePoint m_nextFrameTime;
        Duration m_elapsedTime;
        Duration m_deltaTime;
    };


    class FrameTimer {
    public:
        FrameTimer(uint64_t targetFps)
            : m_targetFps(targetFps),
            m_targetFrameDuration(Duration(targetFps != 0 ? 1'000'000'000 / targetFps : 0)) {
            reset();
        }

        double update() noexcept {
            TimePoint now = Clock::now();

            // If we're behind schedule, skip sleeping entirely
            if (now < m_nextFrameTime) {
                auto remaining = m_nextFrameTime - now;

                // Sleep ~85% of remaining time to avoid oversleep
                if (remaining > std::chrono::microseconds(200)) {
                    auto coarseSleep = remaining - remaining / 7;
                    std::this_thread::sleep_for(coarseSleep);
                }

                // Spin-wait until target time
                while ((now = Clock::now()) < m_nextFrameTime) {
                    std::memory_order_relaxed;
                }
            }

            // Update delta and times
            auto deltaTime = now - m_lastFrameTime;
            m_lastFrameTime = now;
            m_nextFrameTime += m_targetFrameDuration;
            m_elapsedTime += deltaTime;

            return std::chrono::duration<double>(deltaTime).count();
        }

        double getElapsedTimeMs() const {
            return std::chrono::duration<double, std::milli>(m_elapsedTime).count();
        }

        void reset() {
            m_lastFrameTime = Clock::now();
            m_nextFrameTime = m_lastFrameTime + m_targetFrameDuration;
            m_elapsedTime = Duration::zero();
        }

    private:
        using Clock = std::chrono::high_resolution_clock;
        using Duration = std::chrono::nanoseconds;
        using TimePoint = Clock::time_point;

        uint64_t m_targetFps;
        Duration m_targetFrameDuration;
        TimePoint m_lastFrameTime;
        TimePoint m_nextFrameTime;
        Duration m_elapsedTime;
    };
}