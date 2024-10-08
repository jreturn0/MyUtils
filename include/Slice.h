#pragma once

namespace utl {

	template <typename T>
	class Slice
	{
	private:
		T* data;
		size_t size;
	public:
		// begin() and end() functions to allow range-based for loop
		T* begin() { return data; }
		const T* begin() const { return data; }
		T* end() { return data + size; }
		const T* end() const { return data + size; }
		size_t getSize() const { return size; }

		// Constructor to initialize the slice
		Slice(T* data = nullptr, size_t size = 0) : data(data), size(size) {}

		std::span<T> getSpan() const { return { data, size }; }


		T& operator[](size_t index) { return data[index]; }
		auto&& operator[](size_t index) const { return data[index]; }


	};
}