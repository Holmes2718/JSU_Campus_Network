#pragma once

#include <string>

struct HttpResponse {
    bool success = false;
    int statusCode = 0;
    std::string body;
    std::wstring errorMessage;
};

class HttpClient {
public:
    static HttpResponse Head(const std::wstring& host, const std::wstring& path);
    static HttpResponse Get(const std::wstring& host, const std::wstring& path);
};
