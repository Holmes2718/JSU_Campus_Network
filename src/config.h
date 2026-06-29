#pragma once

#include <string>

struct Config {
    std::wstring ssid = L"Campus-WiFi";
    std::wstring gateway = L"192.168.254.17";
    std::wstring username = L"your-account@telecom";
    std::wstring password = L"your-password";
    int checkInterval = 10;
    bool autoStart = false;

    static Config Default();
    bool Load(const std::wstring& path);
    bool Save(const std::wstring& path) const;
};
