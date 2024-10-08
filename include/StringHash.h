#pragma once
#include <string_view>

namespace utl {

	inline constexpr uint32_t Fnv1A32(const std::string_view str)
	{
		constexpr uint32_t prime = 16777619u;
		constexpr uint32_t offset = 2166136261u;
		uint32_t hash = offset;
		for (auto&& c : str)
			hash = (hash ^ static_cast<uint32_t>(c)) * prime;
		return hash;
	}

	inline constexpr uint64_t Fnv1A64(const std::string_view str)
	{
		constexpr uint64_t prime = 1099511628211ull;
		constexpr uint64_t offset = 14695981039346656037ull;
		uint64_t hash = offset;
		for (auto&& c : str)
			hash = (hash ^ static_cast<uint64_t>(c)) * prime;
		return hash;
	}
	//FNV-1a hash for string literals. 64bit version by default
	template<typename T = uint64_t>
	struct StringHash
	{
		T hash;

		constexpr StringHash(const std::string_view str) : hash(std::is_same_v<T, uint64_t> ? Fnv1A64(str) : Fnv1A32(str)) {}

		constexpr operator T()noexcept { return hash; }
	};

	using StringHash64 = StringHash<uint64_t>;

	using StringHash32 = StringHash<uint32_t>;

	inline namespace Literals {
		constexpr StringHash64 operator""_h64(const char* str, size_t len)noexcept { return { std::string_view(str, len) }; }

		constexpr StringHash32 operator""_h32(const char* str, size_t len)noexcept { return{ std::string_view(str, len) }; }
	}

}