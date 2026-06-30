#include "logger.h"
#include "util.h"
#include <filesystem>
#include <windows.h>

Logger::Logger(const std::wstring& folderPath)
    : m_folderPath(folderPath) {
    std::filesystem::create_directories(m_folderPath);
    OpenLogFile();
}

std::wstring Logger::GetDateString() {
    SYSTEMTIME now{};
    GetLocalTime(&now);
    wchar_t buffer[32] = {};
    swprintf_s(buffer, L"%04u-%02u-%02u", now.wYear, now.wMonth, now.wDay);
    return std::wstring(buffer);
}

std::wstring Logger::GetTimeString() {
    SYSTEMTIME now{};
    GetLocalTime(&now);
    wchar_t buffer[32] = {};
    swprintf_s(buffer, L"%04u-%02u-%02u %02u:%02u:%02u", now.wYear, now.wMonth, now.wDay, now.wHour, now.wMinute, now.wSecond);
    return std::wstring(buffer);
}

void Logger::OpenLogFile() {
    std::wstring date = GetDateString();
    if (date == m_currentDate && m_file.is_open()) {
        return;
    }

    m_currentDate = date;
    if (m_file.is_open()) {
        m_file.close();
    }

    std::filesystem::path filePath = std::filesystem::path(m_folderPath) / (m_currentDate + L".log");
    m_file.open(filePath, std::ios::app | std::ios::binary);
}

void Logger::Log(const std::wstring& message) {
    std::lock_guard<std::mutex> lock(m_mutex);
    OpenLogFile();
    if (!m_file.is_open()) {
        return;
    }

    std::wstring line = GetTimeString() + L" " + message + L"\r\n";
    std::string output = WideToUtf8(line);
    m_file.write(output.data(), static_cast<std::streamsize>(output.size()));
    m_file.flush();
}

std::string Logger::ToUtf8(const std::wstring& value) {
    return WideToUtf8(value);
}
