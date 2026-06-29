#pragma once

#include <string>

class Wifi {
public:
    static bool GetCurrentSsid(std::wstring& ssid, std::wstring& errorMessage);
};
