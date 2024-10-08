#pragma once
#include <random>
#include <algorithm>
#include <ranges>
namespace utl {
	// Static functions to generate random number within a range
	class RNG
	{
	private:
		inline static std::random_device rd;
		inline static std::mt19937 gen{ rd() };

	public:

		template<typename T = int>
		static T Range(const T min = 0, const T max = 1)
		{
			static_assert(std::is_arithmetic_v<T>, "Template type must be a number (either integer or floating point)");

			if constexpr (std::is_floating_point_v<T>) {
				// For floating-point types
				std::uniform_real_distribution<T> distribute(min, max);
				return distribute(gen);
			} else {
				// For integer types
				std::uniform_int_distribution<T> distribute(min, max);
				return distribute(gen);
			}
		}

		template<typename T>
		static void Shuffle(T& container)
		{
			std::ranges::shuffle(container, gen);
		}



	};
}