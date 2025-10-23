#pragma once
#include <functional>
#include <string_view>
#include <string>


namespace utl {
    // Heterogeneous string hash function for unordered containers
    //template <typename T>
    //concept StringLike = requires(T a, std::string_view s) {
    //    { std::string_view(a) == s } -> std::convertible_to<bool>;
    //};

    struct TransparentStringHash {
        using is_transparent = void; // Enable heterogeneous lookup
        size_t operator()(std::string_view str) const noexcept {
            return std::hash<std::string_view>{}(str);
        }
        size_t operator()(const std::string& str) const noexcept {
            return std::hash<std::string>{}(str);
        }
        size_t operator()(const char* str) const noexcept {
            return std::hash<std::string_view>{}(str);
        }
        size_t operator()(const std::string_view str) {
            return std::hash<std::string_view>{}(str);
        }
        template <typename T>
            requires std::convertible_to<T, std::string_view>
        std::size_t operator()(const T& s) const noexcept {
            return std::hash<std::string_view>{}(s);
        }


    };
}