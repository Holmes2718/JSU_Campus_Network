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

AuthResult LoginService::DetectAuthStatus(const Config& config, Logger& logger) {
    // GET the gateway root page — if it contains "clientip online" we're already authenticated.
    // This matches the Python reference: "clientip online" in text → already online.

    std::wstring path = L"/";
    HttpResponse getResponse = HttpClient::Get(config.gateway, path);
    if (!getResponse.success) {
        logger.Log(L"HTTP GET 请求失败: " + getResponse.errorMessage);
        return BuildAuthResult(AuthState::Error, L"无法连接认证服务器");
    }

    // Check for "clientip online" — same logic as Python:
    //   if "clientip online" in text: print("已经在线")
    if (getResponse.body.find("clientip online") != std::string::npos) {
        return BuildAuthResult(AuthState::Online, L"已经在线");
    }

    // Gateway reachable but "clientip online" not found — could be captive portal.
    // Double-check by trying an external URL: if external is unreachable,
    // we're definitely behind a captive portal that needs authentication.

    HttpResponse externalResp = HttpClient::Get(L"www.baidu.com", L"/");
    if (!externalResp.success) {
        // External unreachable while gateway works = captive portal = need auth
        return BuildAuthResult(AuthState::NeedAuth, L"需要认证（检测到认证页面）");
    }

    // External reachable too — unlikely with DrCOM captive portal, but treat as online
    return BuildAuthResult(AuthState::Online, L"已经在线");
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
