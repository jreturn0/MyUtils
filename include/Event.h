#pragma once
#include <functional>
#include <vector>
#include <shared_mutex>
#include <memory>
#include <thread>
#include <future>
#include <list>
#include <mutex>
#include <atomic>
#include <algorithm>

namespace utl {


	template<typename... Args>
	class EventHandler {
	public:
		using HandlerFuncType = std::function<void(Args...)>;
		using HandlerIdType = uint64_t;

		explicit EventHandler(const HandlerFuncType& handlerFunc)
			: m_handlerFunc(handlerFunc)
		{
			m_handlerId = m_handlerIdCounter.fetch_add(1);
		}
		// Copy constructor
		EventHandler(const EventHandler& src) : m_handlerFunc(src.m_handlerFunc), m_handlerId(src.m_handlerId) {}

		// Move constructor
		EventHandler(EventHandler&& src) noexcept : m_handlerFunc(std::move(src.m_handlerFunc)), m_handlerId(src.m_handlerId) {}

		// Copy assignment operator
		EventHandler& operator=(const EventHandler& src)
		{
			if (this != &src) {
				m_handlerFunc = src.m_handlerFunc;
				m_handlerId = src.m_handlerId;
			}
			return *this;
		}


		// Move assignment operator
		EventHandler& operator=(EventHandler&& src)
		{
			if (this != &src) {
				m_handlerFunc = std::move(src.m_handlerFunc);
				m_handlerId = src.m_handlerId;
			}
			return *this;
		}

		// Function call operator

		void operator()(Args... params) const
		{
			if (m_handlerFunc)
			{
				m_handlerFunc(params...);
			}
		}

		// Equality operator 
		bool operator==(const EventHandler& other) const
		{
			return m_handlerId == other.m_handlerId;
		}

		// Get the unique handler ID
		HandlerIdType id() const
		{
			return m_handlerId;
		}

	private:
		HandlerFuncType m_handlerFunc;
		HandlerIdType m_handlerId{ 0 };
		inline static std::atomic_uint64_t m_handlerIdCounter = (0);
	};

	template<typename... Args>
	class Event {
	public:
		using HandlerType = EventHandler<Args...>;

		// RAII connection (auto-disconnect)
		class Connection {
		public:
			using Id = typename HandlerType::HandlerIdType;

			Connection() = default;
			Connection(Event* ev, Id id) : m_event(ev), m_id(id) {}
			~Connection() { disconnect(); }

			Connection(const Connection&) = delete;
			Connection& operator=(const Connection&) = delete;

			Connection(Connection&& other) noexcept { *this = std::move(other); }
			Connection& operator=(Connection&& other) noexcept {
				if (this != &other) {
					disconnect();
					m_event = other.m_event;
					m_id = other.m_id;
					other.m_event = nullptr;
					other.m_id = 0;
				}
				return *this;
			}

			bool disconnect() {
				if (m_event && m_id) {
					bool removed = m_event->removeId(m_id);
					m_event = nullptr;
					m_id = 0;
					return removed;
				}
				return false;
			}

			bool valid() const noexcept { return m_event != nullptr; }
			Id   id() const noexcept { return m_id; }

		private:
			Event* m_event{ nullptr };
			Id     m_id{ 0 };
		};

		// RAII connect
		[[nodiscard]] Connection connect(const typename HandlerType::HandlerFuncType& func) {
			return Connection(this, add(func));
		}

		// Default constructor
		Event() {};

		// Copy constructor
		Event(const Event& src) {
			std::shared_lock lock(src.m_handlersLocker);
			m_handlers = src.m_handlers;
		}

		// Move constructor
		Event(Event&& src) {
			std::lock_guard lock(src.m_handlersLocker);
			m_handlers = std::move(src.m_handlers);
		}

		// Copy assignment operator
		Event& operator=(const Event& src) {

			std::scoped_lock lock(m_handlersLocker, src.m_handlersLocker);
			m_handlers = src.m_handlers;
			return *this;
		}

		// Move assignment operator
		Event& operator=(Event&& src) {
			std::scoped_lock lock(m_handlersLocker, src.m_handlersLocker);
			std::swap(m_handlers, src.m_handlers);
			return *this;
		}
		// Add a new handler, returns the handler ID
		[[nodiscard]] HandlerType::HandlerIdType add(const HandlerType& handler)
		{
			std::unique_lock lock(m_handlersLocker);
			m_handlers.emplace_back(handler);
			return handler.id();
		}

		[[nodiscard]] HandlerType::HandlerIdType addOnce(const typename HandlerType::HandlerFuncType& func) {
			auto eventPtr = this; // capture "this" safely

			// Shared pointer to the handler ID so we can capture it inside the lambda
			auto handlerIdPtr = std::make_shared<HandlerType::HandlerIdType>(0);

			HandlerType wrappedHandler{
				[eventPtr, handlerIdPtr, func](Args... args) mutable {
					func(args...);                     // Call actual user function
					eventPtr->removeId(*handlerIdPtr); // Remove self after first call
				}
			};

			*handlerIdPtr = wrappedHandler.id(); // Set the ID after creation

			return add(std::move(wrappedHandler));
		}

		// Add a new handler from a function, returns the handler ID
		[[nodiscard]] inline HandlerType::HandlerIdType add(const HandlerType::HandlerFuncType& handler) {
			return add(HandlerType(handler));
		}
		// Remove a handler by HandlerType, returns true if removed
		bool remove(const HandlerType& handler)
		{
			std::unique_lock lock(m_handlersLocker);
			auto it = std::find(m_handlers.begin(), m_handlers.end(), handler);
			if (it != m_handlers.end())
			{
				m_handlers.erase(it);
				return true;
			}
			return false;
		}
		// Remove a handler by HandlerIdType, returns true if removed
		bool removeId(const HandlerType::HandlerIdType handlerId)
		{
			std::unique_lock lock(m_handlersLocker);
			auto it = std::find_if(m_handlers.begin(), m_handlers.end(), [handlerId](const HandlerType& h) { return h.id() == handlerId; });
			if (it != m_handlers.end())
			{
				m_handlers.erase(it);
				return true;
			}
			return false;
		}

		void clear()
		{
			std::shared_lock lock(m_handlersLocker);
			m_handlers.clear();
		}

		size_t size() const
		{
			std::shared_lock lock(m_handlersLocker);
			return m_handlers.size();
		}

		bool empty() const
		{
			std::shared_lock lock(m_handlersLocker);
			return m_handlers.empty();
		}



		// Call all handlers with the given parameters
		void call(Args... params) const
		{

			HandlerCollectionType handlersCopy = getHandlersCopy();
			callImpl(handlersCopy, params...);
		}
		// Call all handlers asynchronously, returns a future
		std::future<void> callAsync(Args... params) const
		{
			HandlerCollectionType handlersCopy = getHandlersCopy();
			return std::async(std::launch::async, [this, handlersCopy, params...]() {
				callImpl(handlersCopy, params...);
				});
		}

		// Call operator
		inline void operator()(Args... params) const { call(params...); }

		// Add by HandlerType operator, returns the handler ID
		// If using with lambda, you must store the id to remove it later. 
		inline HandlerType::HandlerIdType operator+=(const HandlerType& handler) { return add(handler); }

		// Add by HandlerIdType operator
		// If using with lambda, you must store the id to remove it later. 
		inline HandlerType::HandlerIdType operator+=(const HandlerType::HandlerFuncType& handler) { return add(handler); }

		// Remove by HandlerType operator
		inline bool operator-=(const HandlerType& handler) { return remove(handler); }

		// Remove by HandlerIdType operator
		inline bool operator-=(const HandlerType::HandlerIdType handlerId) { return removeId(handlerId); }





	protected:
		using HandlerCollectionType = std::vector<HandlerType>;

		void callImpl(const HandlerCollectionType& handlers, Args... params) const
		{
			for (const auto& handler : handlers)
			{
				handler(params...);
			}
		}

		HandlerCollectionType getHandlersCopy() const
		{
			std::shared_lock lock(m_handlersLocker);

			// Since the function return value is by copy, 
			// before the function returns (and destruct the lock_guard object),
			// it creates a copy of the m_handlers container.
			return m_handlers;
		}

	private:

		HandlerCollectionType m_handlers;
		mutable std::shared_mutex m_handlersLocker;
	};
}