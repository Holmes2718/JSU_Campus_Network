#include "http.h"
#include "util.h"
#include <windows.h>
#include <winhttp.h>
#include <string>
#include <vector>

static HttpResponse PerformRequest(const std::wstring& host, const std::wstring& path, bool useHead) {
    HttpResponse response;
    HINTERNET session = WinHttpOpen(L"DrcomAutoLogin/1.0", WINHTTP_ACCESS_TYPE_DEFAULT_PROXY, WINHTTP_NO_PROXY_NAME, WINHTTP_NO_PROXY_BYPASS, 0);
    if (!session) {
        response.errorMessage = L"WinHttpOpen 失败。";
        return response;
    }

    HINTERNET connect = WinHttpConnect(session, host.c_str(), INTERNET_DEFAULT_HTTP_PORT, 0);
    if (!connect) {
        response.errorMessage = L"WinHttpConnect 失败。";
        WinHttpCloseHandle(session);
        return response;
    }

    const wchar_t* method = useHead ? L"HEAD" : L"GET";
    HINTERNET request = WinHttpOpenRequest(connect, method, path.c_str(), NULL, WINHTTP_NO_REFERER, WINHTTP_DEFAULT_ACCEPT_TYPES, 0);
    if (!request) {
        response.errorMessage = L"WinHttpOpenRequest 失败。";
        WinHttpCloseHandle(connect);
        WinHttpCloseHandle(session);
        return response;
    }

    BOOL sent = WinHttpSendRequest(request, WINHTTP_NO_ADDITIONAL_HEADERS, 0, WINHTTP_NO_REQUEST_DATA, 0, 0, 0);
    if (!sent) {
        response.errorMessage = L"WinHttpSendRequest 失败。";
        WinHttpCloseHandle(request);
        WinHttpCloseHandle(connect);
        WinHttpCloseHandle(session);
        return response;
    }

    if (!WinHttpReceiveResponse(request, NULL)) {
        response.errorMessage = L"WinHttpReceiveResponse 失败。";
        WinHttpCloseHandle(request);
        WinHttpCloseHandle(connect);
        WinHttpCloseHandle(session);
        return response;
    }

    DWORD statusCode = 0;
    DWORD size = sizeof(statusCode);
    if (WinHttpQueryHeaders(request, WINHTTP_QUERY_STATUS_CODE | WINHTTP_QUERY_FLAG_NUMBER, WINHTTP_HEADER_NAME_BY_INDEX, &statusCode, &size, WINHTTP_NO_HEADER_INDEX)) {
        response.statusCode = static_cast<int>(statusCode);
    }

    if (!useHead) {
        DWORD available = 0;
        while (WinHttpQueryDataAvailable(request, &available) && available > 0) {
            std::vector<char> buffer(available + 1);
            DWORD read = 0;
            if (WinHttpReadData(request, buffer.data(), available, &read) && read > 0) {
                response.body.append(buffer.data(), read);
            } else {
                break;
            }
        }
    }

    response.success = true;
    WinHttpCloseHandle(request);
    WinHttpCloseHandle(connect);
    WinHttpCloseHandle(session);
    return response;
}

HttpResponse HttpClient::Head(const std::wstring& host, const std::wstring& path) {
    return PerformRequest(host, path, true);
}

HttpResponse HttpClient::Get(const std::wstring& host, const std::wstring& path) {
    return PerformRequest(host, path, false);
}
