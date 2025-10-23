#pragma once
#include <string_view>
namespace utl {


    /// Meta-programming utilities for type name extraction and manipulation at compile-time.


    // Returns raw type extracted from the __FUNCSIG__ macro
    template <typename T>
    consteval std::string_view GetFullTypeName() noexcept {
        constexpr std::string_view function = __FUNCSIG__;
        constexpr std::string_view marker = "GetFullTypeName";
        constexpr size_t namePos = function.find(marker);
        constexpr size_t start = function.find('<', namePos);
        constexpr size_t end = function.rfind('>');
        if (namePos == std::string_view::npos || start == std::string_view::npos || end == std::string_view::npos || end <= start)
            return std::string_view{ "<unavailable>" };
        return function.substr(start + 1, end - start - 1);
    }



    // Returns the outermost type without "class/struct/enum/etc." prefix or template parameters, keeps namespaces
    template <typename T>
    consteval std::string_view GetTypeName() noexcept {
        constexpr std::string_view fullName = GetFullTypeName<T>();
        constexpr size_t pos = fullName.find(" ");
        if (pos != std::string_view::npos) {
            return fullName.substr(pos + 1);
        }

        return fullName;


    }
    
    // Returns the outermost type without "class/struct/enum/etc." prefix, template parameters or namespaces
    template <typename T>
    consteval std::string_view GetBaseTypeName() noexcept {
        constexpr std::string_view fullName = GetTypeName<T>();
        constexpr size_t pos = fullName.rfind("::");
        if (pos != std::string_view::npos) {
            return fullName.substr(pos + 2);
        }
        return fullName;
    }

    // Helper struct to hold type information
    struct TypeNameInfo {
        const std::string_view name{};
        const std::string_view baseName{};
        const std::string_view fullName{};
    };

    // Helper struct to hold type information
    template<typename T>
    struct TypeName {
        using Type = T;
        static constexpr std::string_view name = GetTypeName<T>();
        static constexpr std::string_view baseName = GetBaseTypeName<T>();
        static constexpr std::string_view fullName = GetFullTypeName<T>();
        static constexpr TypeNameInfo info{ name, baseName, fullName };
    };
    // Helper struct to hold type information

}