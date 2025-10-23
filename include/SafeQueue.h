#pragma once
#include <condition_variable>
#include <mutex>
#include <queue>
template <typename T>
class SafeQueue
{
public:
    void push(T const& val)
    {
        std::lock_guard queue_lock{ m_queueMutex };
        m_queue.push(val);
        m_queueCv.notify_one();
    }

    T pop()
    {
        std::unique_lock queueLock{ m_queueMutex };
        m_queueCv.wait(queueLock, [&] { return !m_queue.empty(); });
        T ret = m_queue.front();
        m_queue.pop();
        return ret;
    }

    bool tryPop(T& out)
    {
        std::lock_guard queue_lock{ m_queueMutex };
        if (m_queue.empty()) {
            return T{};
        }
        out = m_queue.front();
        m_queue.pop();
        return out;
    }



    bool empty() const
    {
        std::lock_guard<std::mutex> queue_lock{ m_queueMutex };
        return m_queue.empty();
    }

private:
    std::queue<T> m_queue;
    std::condition_variable m_queueCv;
    std::mutex m_queueMutex;
};