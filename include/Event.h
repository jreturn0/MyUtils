#pragma once

#include <atomic>
#include <cstdint>
#include <functional>
#include <memory>
#include <shared_mutex>
#include <unordered_map>
#include <vector>

namespace utl {

    template <typename... Args>
    class EventHandler {
    public:
        using FuncType = std::function<void(Args...)>;
        using IdType = std::uint64_t;

        explicit EventHandler(const FuncType& handlerFunc)
            : m_handlerFunc(handlerFunc) {
            m_handlerId = m_handlerIdCounter.fetch_add(1, std::memory_order_relaxed) + 1;
        }

        explicit EventHandler(FuncType&& handlerFunc)
            : m_handlerFunc(std::move(handlerFunc)) {
            m_handlerId = m_handlerIdCounter.fetch_add(1, std::memory_order_relaxed) + 1;
        }

        EventHandler(const EventHandler&) = default;
        EventHandler(EventHandler&&) noexcept = default;
        EventHandler& operator=(const EventHandler&) = default;
        EventHandler& operator=(EventHandler&&) noexcept = default;

        void operator()(Args... params) const {
            if (m_handlerFunc) {
                m_handlerFunc(std::forward<Args>(params)...);
            }
        }

        bool operator==(const EventHandler& other) const noexcept {
            return m_handlerId == other.m_handlerId;
        }

        IdType getId() const noexcept { return m_handlerId; }

    private:
        FuncType m_handlerFunc;
        IdType m_handlerId{ 0 };
        inline static std::atomic<IdType> m_handlerIdCounter{ 0 };
    };

    template <typename... Args>
    class Event : public std::enable_shared_from_this<Event<Args...>> {
    public:
        using HandlerType = EventHandler<Args...>;
        using HandlerCollectionType = std::vector<HandlerType>;
        using HandlerIndexMapType = std::unordered_map<typename HandlerType::IdType, std::size_t>;

        class Connection {
        public:
            using Id = typename HandlerType::IdType;
            friend class Event<Args...>;

            Connection() = default;
            ~Connection() { disconnect(); }

            Connection(const Connection&) = delete;
            Connection& operator=(const Connection&) = delete;

            Connection(Connection&& other) noexcept { *this = std::move(other); }

            Connection& operator=(Connection&& other) noexcept {
                if (this != &other) {
                    disconnect();
                    m_event = std::move(other.m_event);
                    m_handlerId = other.m_handlerId;
                    other.m_handlerId = 0;
                }
                return *this;
            }

            bool disconnect() {
                if (auto sp = m_event.lock()) {
                    const bool removed = sp->removeImpl(m_handlerId);
                    if (removed) {
                        m_event.reset();
                        m_handlerId = 0;
                    }
                    return removed;
                }
                return false;
            }

            bool isConnected() const { return !m_event.expired() && m_handlerId != 0; }

        private:
            Connection(const std::shared_ptr<Event>& event, Id handlerId)
                : m_event(event), m_handlerId(handlerId) {
            }

            std::weak_ptr<Event> m_event;
            Id m_handlerId{ 0 };
        };

        Event() = default;

        Connection connect(const typename HandlerType::FuncType& handler) {
            std::unique_lock<std::shared_mutex> lock(m_mutex);
            auto id = addImpl(handler);
            return Connection{ this->shared_from_this(), id };
        }

        void operator()(Args... params) const {
            std::shared_lock<std::shared_mutex> lock(m_mutex);
            for (const auto& handler : m_handlers) {
                handler(std::forward<Args>(params)...);
            }
        }

    private:
        typename HandlerType::IdType addImpl(const typename HandlerType::FuncType& handler) {
            HandlerType eventHandler(handler);
            const auto id = eventHandler.getId();

            m_handlers.push_back(std::move(eventHandler));
            m_handlerIndexMap[id] = m_handlers.size() - 1;

            return id;
        }

        bool removeImpl(typename HandlerType::IdType handlerId) {
            std::unique_lock<std::shared_mutex> lock(m_mutex);

            auto it = m_handlerIndexMap.find(handlerId);
            if (it == m_handlerIndexMap.end()) {
                return false;
            }

            const std::size_t index = it->second;
            const std::size_t last = m_handlers.size() - 1;

            if (index != last) {
                std::swap(m_handlers[index], m_handlers[last]);
                m_handlerIndexMap[m_handlers[index].getId()] = index;
            }

            m_handlers.pop_back();
            m_handlerIndexMap.erase(it);

            return true;
        }

        HandlerCollectionType m_handlers;
        HandlerIndexMapType m_handlerIndexMap;
        mutable std::shared_mutex m_mutex;
    };

} // namespace utl