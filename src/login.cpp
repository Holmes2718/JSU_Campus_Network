#include "login.h"
#include "http.h"
#include "util.h"
#include <windows.h>
#include <shellapi.h>
#include <chrono>
#include <algorithm>
#include <cctype>

static AuthResult BuildAuthResult(AuthState state, const std::wstring& message) {
    AuthResult result;
    result.state = state;
    result.message = message;
    return result;
}

static void AppendParameter(std::wstring& query, const std::wstring& key, const std::wstring& value) {
    query += L"&" + key + L"=" + Utf8ToWide(UrlEncode(value));
}

// Case-insensitive substring search (ASCII only — sufficient for DrCOM response markers)
static bool ContainsIgnoreCase(const std::string& haystack, const std::string& needle) {
    if (needle.empty()) return false;
    auto it = std::search(haystack.begin(), haystack.end(),
        needle.begin(), needle.end(),
        [](char a, char b) { return std::tolower(static_cast<unsigned char>(a))
                                   == std::tolower(static_cast<unsigned char>(b)); });
    return it != haystack.end();
}

// DrCOM servers return varied responses across versions / languages.
// Check every known marker for "already online" and "login success".
static bool ParseLoginResponse(const std::string& body, bool& alreadyOnline, bool& success) {
    // ---- Login success ----
    success = body.find("\"result\":1") != std::string::npos
           || body.find("\"result\":\"1\"") != std::string::npos;

    // ---- Already online (multiple languages / formats) ----
    alreadyOnline = ContainsIgnoreCase(body, "clientip online")
                 || ContainsIgnoreCase(body, "client online")
                 || ContainsIgnoreCase(body, "already online")
                 || body.find("已经在线") != std::string::npos
                 || body.find("在线") != std::string::npos
                 || body.find("您已经") != std::string::npos   // "您已经在线了" etc.
                 || body.find("已登录") != std::string::npos;

    return alreadyOnline || success;
}

AuthResult LoginService::PerformLogin(const Config& config, Logger& logger) {
    std::wstring query = L"/drcom/login?callback=dr1003";
    AppendParameter(query, L"DDDDD", config.username);
    AppendParameter(query, L"upass", config.password);
    query += L"&0MKKey=123456&R1=0&R2=&R3=0&R6=0&para=00&v6ip=&terminal_type=1&lang=en&jsVersion=4.2";

    auto now = std::chrono::system_clock::now();
    auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch()).count();
    query += L"&v=" + std::to_wstring(ms);

    HttpResponse response = HttpClient::Get(config.gateway, query);
    if (!response.success) {
        logger.Log(L"HTTP 请求失败: " + response.errorMessage);
        return BuildAuthResult(AuthState::Error, L"网络错误");
    }

    bool alreadyOnline = false;
    bool success = false;
    ParseLoginResponse(response.body, alreadyOnline, success);

    if (success) {
        return BuildAuthResult(AuthState::Online, L"登录成功");
    }
    if (alreadyOnline) {
        return BuildAuthResult(AuthState::Online, L"已经在线");
    }

    // Check for specific error messages from the server
    if (response.body.find("userid error") != std::string::npos) {
        return BuildAuthResult(AuthState::Error, L"账号错误，请检查 config.ini 中的 Username");
    }
    if (response.body.find("passwd error") != std::string::npos) {
        return BuildAuthResult(AuthState::Error, L"密码错误，请检查 config.ini 中的 Password");
    }

    // Unexpected — log first 300 chars of response for debugging
    std::string preview = response.body.substr(0, 300);
    // Replace newlines so the log stays single-line
    std::replace(preview.begin(), preview.end(), '\r', ' ');
    std::replace(preview.begin(), preview.end(), '\n', ' ');
    logger.Log(L"未识别的响应: " + Utf8ToWide(preview));
    return BuildAuthResult(AuthState::Error, L"登录失败（响应未识别）");
}
