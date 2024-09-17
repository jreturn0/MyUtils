#pragma once
#include <numeric>
#include <array>
#include <mutex>


template <typename T, size_t Size = 64>
struct FreeList
{
	static_assert(std::is_trivially_destructible_v<T>, "T must be trivially destructible");
private:
	std::array<T, Size> pool;
	std::array<size_t, Size> freeIndices;
	size_t freeCount;
	std::mutex mtx;

public:


	FreeList() : freeCount(Size)
	{
		std::iota(freeIndices.begin(), freeIndices.end(), 0);
	}

	T* allocate()
	{
		if (freeCount == 0) {
			//return nullptr;
			freeCount = Size;
		}
		return &pool[freeIndices[--freeCount]];
	}

	template <typename... Args>
	T* allocate(Args&&... args)
	{
		if (freeCount == 0) {
			//return nullptr;
			freeCount = Size;
		}
		size_t index = freeIndices[--freeCount];
		void* mem = &pool[index];
		return new (mem) T(std::forward<Args>(args)...);
	}

	void deallocate(T* obj)
	{
		if (obj == nullptr) {
			return;
		}
		obj->~T();
		size_t index = obj - &pool[0];
		freeIndices[freeCount++] = index;
	}

	void clear()
	{
		freeCount = Size;
		for (size_t i = 0; i < Size; ++i) {
			freeIndices[i] = i;
		}
	}

	size_t getFreeCount() const
	{
		return freeCount;
	}
};

