#pragma once

#include <windows.h>
#include <cstdint>

class WaitableTimer {
public:
    WaitableTimer();
    ~WaitableTimer();

    bool IsValid() const;
    bool Start(uint64_t seconds);
    bool Cancel();
    DWORD Wait(DWORD milliseconds = INFINITE) const;
    HANDLE GetHandle() const;

private:
    HANDLE m_timer;
};
