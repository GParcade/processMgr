#pragma once
#include <windows.h>
#include <string>

int get_console_chars_len() {
    CONSOLE_SCREEN_BUFFER_INFO csbi;
    if (!GetConsoleScreenBufferInfo(GetStdHandle(STD_OUTPUT_HANDLE), &csbi))
        return -1;
    else
        return csbi.srWindow.Right - csbi.srWindow.Left;
}
int get_console_chars_heght() {
    CONSOLE_SCREEN_BUFFER_INFO csbi;
    if (!GetConsoleScreenBufferInfo(GetStdHandle(STD_OUTPUT_HANDLE), &csbi))
        return -1;
    else
        return csbi.srWindow.Bottom - csbi.srWindow.Top;
}
inline std::wstring set_pos_in_line(uint32_t y) {
    if (y == 0)return L"\033[999999D";
    return L"\033[999999D\033[" + std::to_wstring(y) + L"C";
}

template < class CharT, class Traits >
constexpr std::basic_ostream< CharT, Traits >& reset_color(std::basic_ostream< CharT, Traits >& os) {
    return os << "\033[0m";
}
std::wstring reset_color() {
    return L"\033[0m";
}

inline std::wstring rgb_color_text(uint8_t r, uint8_t g, uint8_t b) {
    return L"\033[" + (L"38;2;" + std::to_wstring(r) + L";" + std::to_wstring(g) + L";" + std::to_wstring(b) + L"m");
}
inline std::wstring unique_rgb_color(std::wstring str) {
    //xor hash
    uint8_t rgb[3]{0};
    char rgp_pos = 0;
    for (auto& cha : str) {
        for (size_t i = 0; i < sizeof(cha); i++) {
            rgb[rgp_pos++] ^= ((char*)&cha)[i];
            if (rgp_pos == 3)rgp_pos = 0;
        }
    }
    //smooth
    rgb[0] = rgb[0] % 100 + 128;
    rgb[1] = rgb[1] % 100 + 128;
    rgb[2] = rgb[2] % 100 + 128;


    return L"\033[" + (L"38;2;" + std::to_wstring(rgb[0]) + L";" + std::to_wstring(rgb[1]) + L";" + std::to_wstring(rgb[2]) + L"m");
}
inline std::wstring unique_rgb_color(int64_t i) {
    return unique_rgb_color(std::to_wstring(i));
}


void calculate_max_size(const char* value, size_t& len) {
    size_t tmp = std::string(value).size();
    if (len < tmp) len = tmp;
}
void calculate_max_size(const wchar_t* value, size_t& len) {
    size_t tmp = std::wstring(value).size();
    if (len < tmp) len = tmp;
}
template<typename T>
void calculate_max_size(T& value, size_t& len) {
    size_t tmp;
    if constexpr (std::is_same<T, std::string>::value)
        tmp = value.size();
    else if constexpr (std::is_same<T, std::wstring>::value)
        tmp = value.size();
    else
        tmp = std::to_string(value).size();
    if (len < tmp) len = tmp;
}
