#pragma once

#include <string>
#include <sstream>
#include <iomanip>
#include <windows.h>

// ──────────────────────────────────────────────
// Shared string conversion utilities for the project.
// Header-only to avoid adding another .cpp to the build.
// ──────────────────────────────────────────────

inline std::string WideToUtf8(const std::wstring& value) {
    if (value.empty()) {
        return {};
    }
    int sizeNeeded = WideCharToMultiByte(CP_UTF8, 0, value.data(),
        static_cast<int>(value.size()), nullptr, 0, nullptr, nullptr);
    if (sizeNeeded <= 0) {
        return {};
    }
    std::string utf8;
    utf8.resize(sizeNeeded);
    WideCharToMultiByte(CP_UTF8, 0, value.data(),
        static_cast<int>(value.size()), utf8.data(), sizeNeeded, nullptr, nullptr);
    return utf8;
}

inline std::wstring Utf8ToWide(const std::string& value) {
    if (value.empty()) {
        return {};
    }
    int wideSize = MultiByteToWideChar(CP_UTF8, 0, value.c_str(),
        static_cast<int>(value.size()), nullptr, 0);
    if (wideSize <= 0) {
        return {};
    }
    std::wstring result;
    result.resize(wideSize);
    MultiByteToWideChar(CP_UTF8, 0, value.c_str(),
        static_cast<int>(value.size()), result.data(), wideSize);
    return result;
}

// Percent-encode a wide string as UTF-8, preserving unreserved characters.
// Used by both HTTP request building and DrCOM login query construction.
inline std::string UrlEncode(const std::wstring& value) {
    std::string utf8 = WideToUtf8(value);

    std::ostringstream encoded;
    encoded << std::hex;
    for (unsigned char c : utf8) {
        if ((c >= '0' && c <= '9') ||
            (c >= 'A' && c <= 'Z') ||
            (c >= 'a' && c <= 'z') ||
            c == '-' || c == '_' || c == '.' || c == '~') {
            encoded << c;
        } else {
            encoded << '%' << std::uppercase
                     << std::setw(2) << std::setfill('0')
                     << static_cast<int>(c)
                     << std::nouppercase;
        }
    }
    return encoded.str();
}
