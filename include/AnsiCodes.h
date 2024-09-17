#pragma once
#include <string_view>
#include <string>
#include <format>
#include <iostream>
template <typename... Args>
void Print(const std::string_view &fmt, Args &&...args)
{
    std::cout << std::vformat(fmt, std::make_format_args(args...));
}
template <typename T>
void Print(T &&t)
{
    std::cout << t;
}

constexpr std::array<char, 11> SetColor(int id)
{
    // We are constructing the string "\033[38;5;IDm"
    std::array<char, 11> result = {'\033', '[', '3', '8', ';', '5', ';', '0', '0', '0', 'm'};

    if (id > 255 || id < 0)
        id = 0; // Fallback to color 0 if out of range

    // Convert `id` to its individual digits
    int hundreds = id / 100;
    int tens = (id % 100) / 10;
    int ones = id % 10;

    result[7] = '0' + hundreds;
    result[8] = '0' + tens;
    result[9] = '0' + ones;

    return result;
}
namespace ansi {

    // Reset / Normal
    constexpr std::string_view reset = "\033[0m";

    // Text Formatting
    constexpr std::string_view bold = "\033[1m";
    constexpr std::string_view dim = "\033[2m";
    constexpr std::string_view italic = "\033[3m";
    constexpr std::string_view underline = "\033[4m";
    constexpr std::string_view blink = "\033[5m";
    constexpr std::string_view reverse = "\033[7m";
    constexpr std::string_view hidden = "\033[8m";

    // Text Colors
    constexpr std::string_view black = "\033[30m";
    constexpr std::string_view red = "\033[31m";
    constexpr std::string_view green = "\033[32m";
    constexpr std::string_view yellow = "\033[33m";
    constexpr std::string_view blue = "\033[34m";
    constexpr std::string_view magenta = "\033[35m";
    constexpr std::string_view cyan = "\033[36m";
    constexpr std::string_view white = "\033[37m";
    constexpr std::string_view default_color = "\033[39m";

    // Background Colors
    constexpr std::string_view bg_black = "\033[40m";
    constexpr std::string_view bg_red = "\033[41m";
    constexpr std::string_view bg_green = "\033[42m";
    constexpr std::string_view bg_yellow = "\033[43m";
    constexpr std::string_view bg_blue = "\033[44m";
    constexpr std::string_view bg_magenta = "\033[45m";
    constexpr std::string_view bg_cyan = "\033[46m";
    constexpr std::string_view bg_white = "\033[47m";
    constexpr std::string_view default_bg = "\033[49m";

    // Cursor Movement
    inline std::string cursor_up(int n) { return "\033[" + std::to_string(n) + "A"; }
    inline std::string cursor_down(int n) { return "\033[" + std::to_string(n) + "B"; }
    inline std::string cursor_forward(int n) { return "\033[" + std::to_string(n) + "C"; }
    inline std::string cursor_backward(int n) { return "\033[" + std::to_string(n) + "D"; }
    inline std::string cursor_next_line(int n) { return "\033[" + std::to_string(n) + "E"; }
    inline std::string cursor_prev_line(int n) { return "\033[" + std::to_string(n) + "F"; }
    inline std::string cursor_column(int n) { return "\033[" + std::to_string(n) + "G"; }
    inline std::string cursor_position(int n, int m) { return "\033[" + std::to_string(n) + ";" + std::to_string(m) + "H"; }
    constexpr std::string_view save_cursor = "\033[s";
    constexpr std::string_view restore_cursor = "\033[u";
    constexpr std::string_view home_cursor = "\033[H";
    constexpr std::string_view hide_cursor = "\033[?25l";
    constexpr std::string_view show_cursor = "\033[?25h";

    // Clear Screen / Line
    constexpr std::string_view clear_screen = "\033[2J";
    constexpr std::string_view clear_screen_from_cursor = "\033[0J";
    constexpr std::string_view clear_screen_to_cursor = "\033[1J";
    constexpr std::string_view clear_line = "\033[2K";
    constexpr std::string_view clear_line_from_cursor = "\033[0K";
    constexpr std::string_view clear_line_to_cursor = "\033[1K";

} // namespace ansi