#pragma once

#include "config.h"
#include "logger.h"
#include <functional>
#include <string>
#include <windows.h>

class Tray {
public:
    Tray(const Config& config,
         const Logger& logger,
         const std::wstring& configPath,
         std::function<void()> immediateAuth,
         std::function<void()> openLogs,
         std::function<void()> reloadConfig,
         std::function<void()> exitApp);
    ~Tray();

    void Run();

private:
    static LRESULT CALLBACK WindowProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam);
    void CreateWindowClass();
    void CreateTrayIcon();
    void RemoveTrayIcon();
    void ShowContextMenu(HWND hwnd);
    void ApplyAutoStart(bool enable);

    HINSTANCE m_instance;
    HWND m_window;
    std::wstring m_configPath;
    std::wstring m_appName;
    bool m_autoStart;
    const Logger& m_logger;
    std::function<void()> m_immediateAuth;
    std::function<void()> m_openLogs;
    std::function<void()> m_reloadConfig;
    std::function<void()> m_exitApp;
};
