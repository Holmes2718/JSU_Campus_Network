#include "login.h"
#include "http.h"
#include "util.h"
#include <windows.h>
#include <shellapi.h>
#include <chrono>
#include <algorithm>
#include <cctype>
#include <cstdlib>

static AuthResult BuildAuthResult(AuthState state, const std::wstring& message) {
    AuthResult result;
    result.state = state;
    result.message = message;
    return result;
}

static void AppendParameter(std::wstring& query, const std::wstring& key, const std::wstring& value) {
    query += L"&" + key + L"=" + Utf8ToWide(UrlEncode(value));
}

// Extract the integer value of the "result" field from DrCOM JSONP response.
// Response format: dr1003({"result":1,...})
// Returns -1 if the field is not found or unparseable.
static int ExtractResult(const std::string& body) {
    const char marker[] = "\"result\":";
    auto pos = body.find(marker);
    if (pos == std::string::npos) return -1;
    pos += sizeof(marker) - 1;
    // skip whitespace
    while (pos < body.size() && (body[pos] == ' ' || body[pos] == '\t')) ++pos;
    if (pos >= body.size()) return -1;
    // handle both number and quoted-string: 1  or  "1"
    if (body[pos] == '"') {
        ++pos;
        if (pos >= body.size()) return -1;
        int val = 0;
        while (pos < body.size() && body[pos] >= '0' && body[pos] <= '9') {
            val = val * 10 + (body[pos] - '0');
            ++pos;
        }
        return val;
    }
    if (body[pos] >= '0' && body[pos] <= '9') {
        int val = 0;
        while (pos < body.size() && body[pos] >= '0' && body[pos] <= '9') {
            val = val * 10 + (body[pos] - '0');
            ++pos;
        }
        return val;
    }
    return -1;
}

// Check whether the body indicates "already online" via text markers.
// Used as fallback when result != 1 but the user might already be logged in.
static bool ContainsAlreadyOnline(const std::string& body) {
    // Build a lowercase copy for case-insensitive search
    std::string lower = body;
    std::transform(lower.begin(), lower.end(), lower.begin(),
        [](unsigned char c) { return static_cast<char>(std::tolower(c)); });

    return lower.find("clientip online") != std::string::npos
        || lower.find("client online")  != std::string::npos
        || lower.find("already online") != std::string::npos
        || body.find("已经在线") != std::string::npos
        || body.find("在线") != std::string::npos
        || body.find("您已经") != std::string::npos
        || body.find("已登录") != std::string::npos;
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

    // ── Primary: parse the structured JSONP response ──
    int resultVal = ExtractResult(response.body);

    if (resultVal == 1) {
        // result=1 means the server accepted the request:
        // either login succeeded or we were already logged in.
        return BuildAuthResult(AuthState::Online, L"已经在线");
    }

    if (resultVal == 0) {
        // result=0 means the request was rejected.
        // Check for known error codes in the message.
        if (response.body.find("userid error") != std::string::npos) {
            return BuildAuthResult(AuthState::Error, L"账号错误，请检查 config.ini 中的 Username");
        }
        if (response.body.find("passwd error") != std::string::npos) {
            return BuildAuthResult(AuthState::Error, L"密码错误，请检查 config.ini 中的 Password");
        }

        // Even with result=0, the body may contain "already online" text.
        if (ContainsAlreadyOnline(response.body)) {
            return BuildAuthResult(AuthState::Online, L"已经在线");
        }

        // Unknown rejection reason — log the response for diagnostics.
        std::string preview = response.body.substr(0, 300);
        std::replace(preview.begin(), preview.end(), '\r', ' ');
        std::replace(preview.begin(), preview.end(), '\n', ' ');
        logger.Log(L"服务器拒绝: " + Utf8ToWide(preview));
        return BuildAuthResult(AuthState::Error, L"登录失败（服务器返回 result=0）");
    }

    // ── Fallback: could not parse result field ──
    // (older / non-standard DrCOM versions that don't return JSONP)
    if (ContainsAlreadyOnline(response.body)) {
        return BuildAuthResult(AuthState::Online, L"已经在线");
    }

    std::string preview = response.body.substr(0, 300);
    std::replace(preview.begin(), preview.end(), '\r', ' ');
    std::replace(preview.begin(), preview.end(), '\n', ' ');
    logger.Log(L"未识别的响应: " + Utf8ToWide(preview));
    return BuildAuthResult(AuthState::Error, L"登录失败（响应未识别）");
}
