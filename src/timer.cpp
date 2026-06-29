#include "timer.h"

WaitableTimer::WaitableTimer() {
    m_timer = CreateWaitableTimerW(NULL, TRUE, NULL);
}

WaitableTimer::~WaitableTimer() {
    if (m_timer) {
        CloseHandle(m_timer);
    }
}

bool WaitableTimer::IsValid() const {
    return m_timer != NULL;
}

bool WaitableTimer::Start(uint64_t seconds) {
    if (!m_timer) {
        return false;
    }

    LARGE_INTEGER dueTime;
    dueTime.QuadPart = -static_cast<LONGLONG>(seconds * 10000000ULL);
    return SetWaitableTimer(m_timer, &dueTime, 0, NULL, NULL, FALSE) != 0;
}

bool WaitableTimer::Cancel() {
    if (!m_timer) {
        return false;
    }
    return CancelWaitableTimer(m_timer) != 0;
}

DWORD WaitableTimer::Wait(DWORD milliseconds) const {
    if (!m_timer) {
        return WAIT_FAILED;
    }
    return WaitForSingleObject(m_timer, milliseconds);
}

HANDLE WaitableTimer::GetHandle() const {
    return m_timer;
}
