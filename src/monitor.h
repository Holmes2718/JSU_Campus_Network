#pragma once

#include "config.h"
#include "logger.h"
#include "timer.h"
#include <atomic>
#include <thread>
#include <mutex>
#include <windows.h>

class Monitor {
public:
    Monitor(const Config& config, Logger& logger);
    ~Monitor();

    void Start();
    void Stop();
    void RequestImmediateAuth();
    void ReloadConfig(const Config& config);

private:
    void WorkerThread();
    Config m_config;
    Logger& m_logger;
    std::atomic<bool> m_running;
    std::atomic<bool> m_immediateAuth;
    std::thread m_thread;
    HANDLE m_stopEvent;
    HANDLE m_wakeEvent;
    WaitableTimer m_timer;
    std::mutex m_configMutex;
};
