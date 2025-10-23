#pragma once
#include <string_view>


namespace utl {
    namespace details {
        inline constexpr uint32_t Fnv1A32(const std::string_view str) noexcept
        {
            constexpr uint32_t prime = 16777619u;
            constexpr uint32_t offset = 2166136261u;
            uint32_t hash = offset;
            for (auto&& c : str)
                hash = (hash ^ static_cast<uint32_t>(c)) * prime;
            return hash;
        }

        inline constexpr uint64_t Fnv1A64(const std::string_view str) noexcept
        {
            constexpr uint64_t prime = 1099511628211ull;
            constexpr uint64_t offset = 14695981039346656037ull;
            uint64_t hash = offset;
            for (auto&& c : str)
                hash = (hash ^ static_cast<uint64_t>(c)) * prime;
            return hash;
        }

    }



    //FNV-1a hash for string literals. 32bit version
    struct StringHash32
    {
        uint32_t hash;
        std::string_view strView;
        constexpr StringHash32(std::string_view str) : hash(details::Fnv1A32(str)), strView(str) {}

        template <size_t N>
        constexpr StringHash32(const char(&str)[N])
            : hash(details::Fnv1A32(std::string_view(str, N - 1))) {
        }



        constexpr operator uint32_t() noexcept { return hash; }
    };




    //FNV-1a hash for string literals. 64bit version
    struct StringHash64
    {
        uint64_t hash{};
        std::string_view strView{};

        constexpr StringHash64() noexcept = default;
        constexpr StringHash64(std::string_view str) : hash(details::Fnv1A64(str)), strView(str) {}
        template <size_t N>
        constexpr StringHash64(const char(&str)[N]) : hash(details::Fnv1A64(std::string_view(str, N - 1))), strView(str, N - 1) {}
        constexpr StringHash64(uint64_t prehashed) : hash(prehashed), strView{} {}

        constexpr bool operator==(const StringHash64& other) const noexcept { return hash == other.hash; }
        constexpr bool operator!=(const StringHash64& other) const noexcept { return hash != other.hash; }

        constexpr operator uint64_t() noexcept { return hash; }
        constexpr operator uint64_t() const noexcept { return hash; }
    };

    // Default to FNV-1a 64bit version
    using StringHash = StringHash64;

    struct StringHashOp {
        auto operator()(const StringHash64& h) const noexcept {
            return h.hash;
        }
    };


    namespace hashLiterals {
        constexpr StringHash64 operator""_h64(const char* str, size_t len) noexcept { return { std::string_view(str, len) }; }

        constexpr StringHash32 operator""_h32(const char* str, size_t len) noexcept { return{ std::string_view(str, len) }; }
        // Default to 64bit version
        constexpr StringHash operator""_hash(const char* str, size_t len) noexcept { return { std::string_view(str, len) }; }
    }





}