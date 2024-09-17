#pragma once
#include <array>
#include <functional>
#include <span>
#include "Slice.h"
template <typename T, size_t S>
struct StackedSliceArray
{
private:
	std::array<T, S> data{};
	std::array<size_t, S> map{};
	size_t dataSize{ 0 };
	size_t currentSlices{ 0 };



public:



	// Add a new slice to the contiguous array
	bool add(const std::initializer_list<T>& slice)
	{
		if ((dataSize + slice.size()) > S) return false;
		size_t start = dataSize;
		for (const auto& elem : slice) {
			data[dataSize++] = elem;
		}
		map[currentSlices++] = start;
		return true;
	}



	template <typename... Args>
	bool add(Args&&... args)
	{
		// Check if the number of arguments fits within the available space
		if ((dataSize + sizeof...(args)) > S) return false;
		size_t start = dataSize;
		// Use fold expression to add each argument into the data array
		((data[dataSize++] = std::forward<Args>(args)), ...);
		// Store the start index of the current slice
		map[currentSlices++] = start;
		return true;
	}



	// set a specific slice by index
	bool set(size_t index, const std::initializer_list<T>& slice)
	{
		if (index >= currentSlices) return false;
		size_t start = map[index];
		size_t end = (index + 1 < currentSlices) ? map[index + 1] : dataSize;
		size_t newSize = std::min(slice.size(), end - start);
		std::copy(slice.begin(), slice.begin() + newSize, data.begin() + start);
		return true;
	}
	template <typename... Args>
	bool set(size_t index, Args&&... args)
	{
		// Check if the index is valid
		if (index >= currentSlices) return false;

		// get the start and end positions of the slice
		size_t start = map[index];
		size_t end = (index + 1 < currentSlices) ? map[index + 1] : dataSize;

		// Calculate the size of the current slice and the size of the new values
		size_t newSize = std::min(sizeof...(args), end - start);

		// set the values in the data array with the provided arguments
		size_t i = 0;
		((i < newSize ? data[start + i++] = std::forward<Args>(args) : void()), ...);

		return true;
	}



	Slice<T> getSlice(size_t index)
	{
		if (index >= currentSlices) return { nullptr,size_t(-1ll) };
		const size_t start = map[index];
		const size_t end = (index + 1 < currentSlices) ? map[index + 1] : dataSize;
		const size_t length = end - start + 1;
		auto ptr = &data[start];
		return { ptr,length };


	}

	// Access a specific slice by index using std::span
	std::span<T> get(size_t index)
	{
		if (index >= currentSlices) return {};
		size_t start = map[index];
		size_t end = (index + 1 < currentSlices) ? map[index + 1] : dataSize;
		T* startPtr = data.data() + start;
		size_t length = end - start;
		std::span<T> result(startPtr, length); // Separate the construction
		return result; // Return the span
	}

	template <size_t N>
	std::array<T*, N> get(size_t index)
	{
		if (index >= currentSlices) return {};
		size_t start = map[index];
		size_t end = (index + 1 < currentSlices) ? map[index + 1] : dataSize;
		std::array<T*, N> result;
		for (size_t i = 0; i < N; ++i) {
			result[i] = &data[start + i];
		}
		return result;
	}

	std::span<T> operator[](const size_t index) const
	{
		return get(index);
	}

	void foreach(std::function<void(std::span<T>)> func)
	{
		for (size_t i = 0; i < currentSlices; ++i) {
			func(get(i));
		}
	}


};
