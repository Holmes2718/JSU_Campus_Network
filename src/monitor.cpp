#include "monitor.h"
#include "wifi.h"
#include "network.h"
#include "login.h"

Monitor::Monitor(const Config& config, Logger& logger)
    : m_config(config), m_logger(logger), m_running(false), m_immediateAuth(false), m_stopEvent(NULL), m_wakeEvent(NULL) {
}

Monitor::~Monitor() {
    Stop();
}

void Monitor::Start() {
    if (m_running.load()) {
        return;
    }

    m_stopEvent = CreateEventW(NULL, TRUE, FALSE, NULL);
    m_wakeEvent = CreateEventW(NULL, FALSE, FALSE, NULL);
    if (!m_stopEvent || !m_wakeEvent) {
        m_logger.Log(L"创建监控事件失败");
        return;
    }

    m_running.store(true);
    m_thread = std::thread([this]() {
        WorkerThread();
    });
}

void Monitor::Stop() {
    if (!m_running.load()) {
        return;
    }

    m_running.store(false);
    if (m_stopEvent) {
        SetEvent(m_stopEvent);
    }
    if (m_thread.joinable()) {
        m_thread.join();
    }

    if (m_stopEvent) {
        CloseHandle(m_stopEvent);
        m_stopEvent = NULL;
    }
    if (m_wakeEvent) {
        CloseHandle(m_wakeEvent);
        m_wakeEvent = NULL;
    }
}

void Monitor::RequestImmediateAuth() {
    m_immediateAuth.store(true);
    if (m_wakeEvent) {
        SetEvent(m_wakeEvent);
    }
}

void Monitor::ReloadConfig(const Config& config) {
    std::lock_guard<std::mutex> lock(m_configMutex);
    m_config = config;
    m_logger.Log(L"配置已加载");
    if (m_wakeEvent) {
        SetEvent(m_wakeEvent);
    }
}

void Monitor::WorkerThread() {
    // Track last state to avoid log spam
    bool wasOnTarget = false;
    std::wstring lastStatusMsg;

    while (m_running.load()) {
        Config config;
        {
            std::lock_guard<std::mutex> lock(m_configMutex);
            config = m_config;
        }

        std::wstring ssid;
        std::wstring wifiError;
        std::wstring reachableError;
        bool onTarget = false;

        // Path 1: WiFi SSID matching
        if (Wifi::GetCurrentSsid(ssid, wifiError) && ssid == config.ssid) {
            onTarget = true;
        }
        // Path 2: Wired — check if DrCOM server is reachable (like Python's wait_gateway)
        else if (Network::IsHostReachable(config.gateway, reachableError)) {
            onTarget = true;
        }

        if (!onTarget) {
            std::wstring statusMsg;
            if (!ssid.empty() && ssid != config.ssid) {
                // Connected to a WiFi that is NOT the target SSID
                statusMsg = L"当前SSID: " + ssid + L"（非目标网络），等待进入目标网络...";
            } else if (!ssid.empty() && ssid == config.ssid) {
                // Should not happen (would have set onTarget=true), but be defensive
                statusMsg = L"SSID匹配但网络状态异常，等待...";
            } else if (!reachableError.empty()) {
                // WiFi not connected (or check failed) AND DrCOM server unreachable
                if (!wifiError.empty()) {
                    statusMsg = L"WiFi检测失败(" + wifiError + L")，且DrCOM服务器 "
                              + config.gateway + L" 不可达，等待...";
                } else {
                    statusMsg = L"未连接WiFi，DrCOM服务器 " + config.gateway
                              + L" 不可达（等待有线网络接入）...";
                }
            } else {
                statusMsg = L"未检测到目标网络（SSID或服务器可达性检查均未通过），等待...";
            }

            // Only log on state change
            if (statusMsg != lastStatusMsg) {
                m_logger.Log(statusMsg);
                lastStatusMsg = statusMsg;
            }
            wasOnTarget = false;

            if (m_timer.IsValid() && m_timer.Start(30)) {
                const HANDLE idleWaitHandles[3] = { m_stopEvent, m_wakeEvent, m_timer.GetHandle() };
                DWORD index = WaitForMultipleObjects(3, idleWaitHandles, FALSE, INFINITE);
                if (index == WAIT_OBJECT_0) {
                    break;
                }
            } else {
                Sleep(30000);
            }
            continue;
        }

        // Entered target network — log only on transition
        if (!wasOnTarget) {
            m_logger.Log(L"已连接到校园网，开始检测认证状态...");
            wasOnTarget = true;
            lastStatusMsg.clear();
        }

        // Check auth status via login endpoint (the most reliable way).
        // PerformLogin serves as both detection and login:
        //   "已经在线" / "登录成功" → online,  "登录失败" → need retry
        AuthResult authResult = LoginService::PerformLogin(config, m_logger);
        if (authResult.state == AuthState::Online) {
            if (authResult.message != m_lastOnlineMsg) {
                m_logger.Log(authResult.message);
                m_lastOnlineMsg = authResult.message;
            }
        } else {
            m_logger.Log(authResult.message);
        }

        if (m_immediateAuth.exchange(false)) {
            continue;
        }

        // Wait for next check interval
        if (m_timer.IsValid() && m_timer.Start(static_cast<uint64_t>(config.checkInterval))) {
            const HANDLE retryHandles[3] = { m_stopEvent, m_wakeEvent, m_timer.GetHandle() };
            DWORD waitIndex = WaitForMultipleObjects(3, retryHandles, FALSE, INFINITE);
            if (waitIndex == WAIT_OBJECT_0) {
                break;
            }
        } else {
            Sleep(config.checkInterval * 1000);
        }
    }
}
