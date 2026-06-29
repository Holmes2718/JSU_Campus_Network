#include "config.h"
#include <windows.h>
#include <filesystem>

static std::wstring ReadString(const std::wstring& path, const std::wstring& section, const std::wstring& key, const std::wstring& defaultValue) {
    wchar_t buffer[512] = {};
    GetPrivateProfileStringW(section.c_str(), key.c_str(), defaultValue.c_str(), buffer, static_cast<DWORD>(std::size(buffer)), path.c_str());
    return std::wstring(buffer);
}

static int ReadInt(const std::wstring& path, const std::wstring& section, const std::wstring& key, int defaultValue) {
    return static_cast<int>(GetPrivateProfileIntW(section.c_str(), key.c_str(), defaultValue, path.c_str()));
}

Config Config::Default() {
    return Config();
}

bool Config::Load(const std::wstring& path) {
    if (!std::filesystem::exists(path)) {
        return false;
    }

    ssid = ReadString(path, L"Campus", L"SSID", ssid);
    gateway = ReadString(path, L"Campus", L"Gateway", gateway);
    username = ReadString(path, L"Campus", L"Username", username);
    password = ReadString(path, L"Campus", L"Password", password);
    checkInterval = ReadInt(path, L"Campus", L"CheckInterval", checkInterval);
    autoStart = ReadInt(path, L"Campus", L"AutoStart", 0) != 0;

    if (checkInterval <= 0) {
        checkInterval = 2;
    }

    return true;
}

bool Config::Save(const std::wstring& path) const {
    std::filesystem::path filePath(path);
    if (filePath.has_parent_path()) {
        std::filesystem::create_directories(filePath.parent_path());
    }

    bool result = true;
    result &= WritePrivateProfileStringW(L"Campus", L"SSID", ssid.c_str(), path.c_str()) != 0;
    result &= WritePrivateProfileStringW(L"Campus", L"Gateway", gateway.c_str(), path.c_str()) != 0;
    result &= WritePrivateProfileStringW(L"Campus", L"Username", username.c_str(), path.c_str()) != 0;
    result &= WritePrivateProfileStringW(L"Campus", L"Password", password.c_str(), path.c_str()) != 0;
    result &= WritePrivateProfileStringW(L"Campus", L"CheckInterval", std::to_wstring(checkInterval).c_str(), path.c_str()) != 0;
    result &= WritePrivateProfileStringW(L"Campus", L"AutoStart", autoStart ? L"1" : L"0", path.c_str()) != 0;
    return result;
}
