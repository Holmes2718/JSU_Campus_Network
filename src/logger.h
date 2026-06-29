#pragma once

#include <string>
#include <fstream>
#include <mutex>

class Logger {
public:
    explicit Logger(const std::wstring& folderPath);
    void Log(const std::wstring& message);

private:
    std::wstring m_folderPath;
    std::wstring m_currentDate;
    std::ofstream m_file;
    std::mutex m_mutex;

    void OpenLogFile();
    static std::string ToUtf8(const std::wstring& value);
    static std::wstring GetDateString();
    static std::wstring GetTimeString();
};
