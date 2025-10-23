#pragma once
#include "SafeQueue.h"
#include <functional>
#include <future>
#include <memory>
#include <thread>
#include  <type_traits>
#include <vector>
namespace utl {







    class ThreadPool {
    public:
        ThreadPool(size_t threadCount = std::min(4u, std::thread::hardware_concurrency() - 1)) {
            for (size_t i = 0; i < threadCount; ++i) {
                m_workers.emplace_back(&ThreadPool::worker, this);
            }
        }
        ThreadPool(ThreadPool&) = delete;
        ThreadPool(const ThreadPool&) = delete;
        ThreadPool& operator=(ThreadPool&&) = delete;
        ThreadPool& operator=(const ThreadPool&) = delete;
        ~ThreadPool() {
            m_shutdown.store(true);
            m_condition.notify_all();
            for (auto& thread : m_workers) {
                if (thread.joinable())
                    thread.join();
            }
        }

        template <class F, class... Args>
        auto enqueue(F&& f, Args&&... args) -> std::future<std::invoke_result_t<F, Args...>>
        {
            using returnType = std::invoke_result_t<F, Args... >;
            auto task = std::make_shared<std::packaged_task<returnType()>>(
                [f = std::forward<F>(f), ...args = std::forward<Args>(args)]() mutable -> returnType {
                    return std::invoke(std::move(f), std::move(args)...);
                }
            );
            std::future<returnType> future = task->get_future();
            {
                std::unique_lock  lock(m_guard);
                if (m_shutdown.load())
                    throw std::runtime_error("enqueue on stopped ThreadPool");
                m_tasks.emplace_back([task]() { (*task)(); });
            }
            m_condition.notify_one();
            return future;
        }



        // Batch process a container/range with a function in parallel using the thread pool.
        // Supports functions returning either a value or void.
        // If F returns void: returns std::vector<std::future<void>> (just wait on them).
        // If F returns R: returns std::vector<std::future<std::vector<R>>> (batched results).
        template<typename F, typename C>
        auto batchContainer(F&& function, C& container, size_t minBatchSize = 0, size_t maxThreads = 0)
            -> std::conditional_t<
            std::is_void_v<std::invoke_result_t<F&, std::remove_reference_t<decltype(*std::begin(container))>>>,
            std::vector<std::future<void>>,
            std::vector<std::future<std::vector<std::invoke_result_t<F&, std::remove_reference_t<decltype(*std::begin(container))>>>>>
            >
        {
            using Value = std::remove_reference_t<decltype(*std::begin(container))>; 
            using R = std::invoke_result_t<F&, Value>;
            using FutureType = std::conditional_t<std::is_void_v<R>, std::future<void>, std::future<std::vector<R>>>;
            std::vector<FutureType> futures;

            const size_t totalItems = static_cast<size_t>(std::distance(std::begin(container), std::end(container)));
            if (totalItems == 0) {
                return futures;
            }

            const size_t threadLimit = maxThreads == 0 ? threadCount() : maxThreads;

            size_t batchCount = 0;
            if (minBatchSize > 0) {
                batchCount = (totalItems + minBatchSize - 1) / minBatchSize; // ceiling div
                batchCount = std::min(batchCount, threadLimit);
            }
            else {
                batchCount = std::min(threadLimit, totalItems);
            }
            if (batchCount == 0) {
                // Fallback: run everything on one task if no threads available
                batchCount = 1;
            }

            const size_t base = totalItems / batchCount;
            const size_t rem = totalItems % batchCount;

            auto beginIt = std::begin(container);

            for (size_t i = 0; i < batchCount; ++i) {
                const size_t startIndex = i * base + std::min(i, rem);
                const size_t count = base + (i < rem ? 1 : 0);
                const size_t endIndex = startIndex + count;

                auto startIt = std::next(beginIt, static_cast<std::ptrdiff_t>(startIndex));
                auto endIt = std::next(beginIt, static_cast<std::ptrdiff_t>(endIndex));

                if constexpr (std::is_void_v<R>) {
                    futures.emplace_back(enqueue([&, startIt, endIt]() {
                        for (auto it = startIt; it != endIt; ++it) {
                            std::invoke(function, *it);
                        }
                        }));
                }
                else {
                    futures.emplace_back(enqueue([&, startIt, endIt]() -> std::vector<R> {
                        std::vector<R> results;
                        results.reserve(count);
                        for (auto it = startIt; it != endIt; ++it) {
                            results.push_back(std::invoke(function, *it));
                        }
                        return results;
                        }));
                }
            }

            return std::move(futures);
        }



        // Get number of threads
        inline size_t threadCount() const {
            return m_workers.size();
        }

        inline size_t availableThreads() const {
            return m_workers.size() - m_activeTasks.load();
        }

        // Get number of pending tasks
        size_t pendingTaskCount() const {
            std::unique_lock lock(m_guard);
            return m_tasks.size();
        }

        void waitForIdle() {
            std::unique_lock lock(m_guard);
            m_idleCondition.wait(lock, [this] {
                return m_tasks.empty() && m_activeTasks.load() == 0;
                });
        }

    private:
        std::atomic_bool m_shutdown = false;
        std::vector<std::thread> m_workers{};
        std::deque<std::function<void()>> m_tasks{};
        mutable std::mutex m_guard{};
        std::condition_variable m_condition{};
        std::atomic_size_t m_activeTasks{ 0 };
        std::condition_variable m_idleCondition{};

        void worker() {
            for (;;) {
                std::function<void()> task;
                {
                    std::unique_lock lock(m_guard);
                    m_condition.wait(lock, [this] { return m_shutdown.load() || !m_tasks.empty(); });
                    if (m_shutdown.load() && m_tasks.empty())
                        return;
                    if (m_tasks.empty())
                        continue;
                    task.swap(m_tasks.front());
                    m_tasks.pop_front();
                }
                m_activeTasks.fetch_add(1, std::memory_order_relaxed);
                task();
                m_activeTasks.fetch_sub(1, std::memory_order_relaxed);

                // Notify if all tasks are done
                if (m_activeTasks.load(std::memory_order_relaxed) == 0) {
                    std::unique_lock lock(m_guard);
                    m_idleCondition.notify_all();
                }
            }
        }

    };

}