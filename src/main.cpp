#include "config.h"
#include "logger.h"
#include "monitor.h"
#include "tray.h"
#include <windows.h>
#include <shellapi.h>
#include <filesystem>
#include <memory>

#pragma comment(linker, "/SUBSYSTEM:WINDOWS")

static std::wstring GetExecutableFolder() {
    wchar_t buffer[MAX_PATH] = {};
    if (GetModuleFileNameW(NULL, buffer, MAX_PATH) == 0) {
        return L".";
    }
    std::filesystem::path path(buffer);
    return path.parent_path().wstring();
}

int WINAPI wWinMain(HINSTANCE /*hInstance*/, HINSTANCE /*hPrevInstance*/, PWSTR /*pCmdLine*/, int /*nCmdShow*/) {
    const std::wstring exeFolder = GetExecutableFolder();
    const std::wstring configPath = exeFolder + L"\\config.ini";
    const std::wstring logFolder = exeFolder + L"\\logs";

    Config config;
    if (!config.Load(configPath)) {
        config = Config::Default();
        config.Save(configPath);
    }

    Logger logger(exeFolder);

    std::shared_ptr<Monitor> monitor = std::make_shared<Monitor>(config, logger);
    monitor->Start();

    Tray tray(
        config,
        logger,
        configPath,
        [monitor]() {
            monitor->RequestImmediateAuth();
        },
        [logFolder]() {
            SHELLEXECUTEINFOW info = {};
            info.cbSize = sizeof(info);
            info.fMask = SEE_MASK_FLAG_NO_UI;
            info.lpVerb = L"open";
            info.lpFile = logFolder.c_str();
            info.nShow = SW_SHOWNORMAL;
            ShellExecuteExW(&info);
        },
        [&configPath, &logger, monitor]() {
            Config newConfig;
            if (newConfig.Load(configPath)) {
                monitor->ReloadConfig(newConfig);
                logger.Log(L"配置重新加载");
            } else {
                logger.Log(L"重新加载配置失败");
            }
        },
        [&monitor, &logger]() {
            logger.Log(L"收到退出命令");
            monitor->Stop();
        }
    );

    tray.Run();
    logger.Log(L"程序退出");
    return 0;
}
