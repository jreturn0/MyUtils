#pragma once
#include <atomic>
#include <vector>
#include  <cassert>
#include <array>
//Fixed size, contiguous, thread safe queue
template <typename T, size_t Size>
class FixedQueue
{
private:
	std::array<T, Size> buffer;
	mutable std::mutex mtx;
	size_t head; 
	size_t tail;

public:
	const size_t capacity = Size;
	FixedQueue() : head(0), tail(0) {}

	bool enqueue(const T& item)
	{
		std::lock_guard<std::mutex> lock(mtx);
		auto current = tail;
		size_t nextTail = (tail + 1) % Size;
		if (nextTail == head) {
			return false; // Queue is full
		}
		buffer[current] = item;
		tail = nextTail;
		return true;
	}
	bool enqueueUnsafe(const T& item)
	{
		auto current = tail;
		size_t nextTail = (tail + 1) % Size;
		if (nextTail == head) {
			return false; // Queue is full
		}
		buffer[current] = item;
		tail = nextTail;
		return true;
	}

	bool dequeue(T& item)
	{
		std::lock_guard<std::mutex> lock(mtx);
		if (head == tail) {
			return false; // Queue is empty
		}
		item = buffer[head];
		head = (head + 1) % Size;
		return true;
	}
	bool dequeueUnsafe(T& item)
	{
		if (head == tail) {
			return false; // Queue is empty
		}
		item = buffer[head];
		head = (head + 1) % Size;
		return true;
	}

	std::lock_guard <std::mutex> lockGuard() const
	{
		return std::lock_guard<std::mutex>(mtx);
	}
	void lock() const
	{
		mtx.lock();
	}

	void unlock() const
	{
		mtx.unlock();
	}



	bool isEmpty() const
	{
		std::lock_guard<std::mutex> lock(mtx);
		return head == tail;
	}


	void swap(FixedQueue& other) noexcept
	{
		if (this == &other) return;

		std::scoped_lock lock(mtx, other.mtx);
		buffer.swap(other.buffer);
		std::swap(head, other.head);
		std::swap(tail, other.tail);
	}
};