#include "login.h"
#include "http.h"
#include "util.h"
#include <windows.h>
#include <shellapi.h>
#include <chrono>

static AuthResult BuildAuthResult(AuthState state, const std::wstring& message) {
    AuthResult result;
    result.state = state;
    result.message = message;
    return result;
}

static void AppendParameter(std::wstring& query, const std::wstring& key, const std::wstring& value) {
    query += L"&" + key + L"=" + Utf8ToWide(UrlEncode(value));
}

static bool ParseLoginResponse(const std::string& text, bool& alreadyOnline, bool& success) {
    alreadyOnline = text.find("clientip online") != std::string::npos;
    success = text.find("\"result\":1") != std::string::npos;
    return alreadyOnline || success;
}

AuthResult LoginService::PerformLogin(const Config& config, Logger& logger) {
    logger.Log(L"使用静默认证方式，不会自动打开浏览器");

    std::wstring query = L"/drcom/login?callback=dr1003";
    AppendParameter(query, L"DDDDD", config.username);
    AppendParameter(query, L"upass", config.password);
    query += L"&0MKKey=123456&R1=0&R2=&R3=0&R6=0&para=00&v6ip=&terminal_type=1&lang=en&jsVersion=4.2";

    auto now = std::chrono::system_clock::now();
    auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch()).count();
    query += L"&v=" + std::to_wstring(ms);

    HttpResponse response = HttpClient::Get(config.gateway, query);
    if (!response.success) {
        logger.Log(L"登录请求失败: " + response.errorMessage);
        return BuildAuthResult(AuthState::Error, L"登录请求失败");
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

    return BuildAuthResult(AuthState::Error, L"登录失败");
}
