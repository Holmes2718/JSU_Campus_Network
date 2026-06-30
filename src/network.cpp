#include "network.h"
#include <winsock2.h>
#include <ws2tcpip.h>
#include <windows.h>
#include <iphlpapi.h>
#include <string>
#include <vector>

#pragma comment(lib, "Iphlpapi.lib")
#pragma comment(lib, "Ws2_32.lib")

namespace {
    struct WinsockInit {
        WinsockInit() {
            WSADATA wsaData;
            WSAStartup(MAKEWORD(2, 2), &wsaData);
        }
        ~WinsockInit() {
            WSACleanup();
        }
    };
    static WinsockInit g_winsockInit;
}

bool Network::GetDefaultGateway(std::wstring& gateway, std::wstring& errorMessage) {
    gateway.clear();
    errorMessage.clear();

    ULONG flags = GAA_FLAG_SKIP_ANYCAST | GAA_FLAG_SKIP_MULTICAST | GAA_FLAG_SKIP_DNS_SERVER;
    ULONG outBufLen = 15000;
    std::vector<BYTE> buffer(outBufLen);
    PIP_ADAPTER_ADDRESSES pAddresses = reinterpret_cast<PIP_ADAPTER_ADDRESSES>(buffer.data());
    ULONG ret = GetAdaptersAddresses(AF_INET, flags, NULL, pAddresses, &outBufLen);
    if (ret == ERROR_BUFFER_OVERFLOW) {
        buffer.resize(outBufLen);
        pAddresses = reinterpret_cast<PIP_ADAPTER_ADDRESSES>(buffer.data());
        ret = GetAdaptersAddresses(AF_INET, flags, NULL, pAddresses, &outBufLen);
    }

    if (ret != NO_ERROR) {
        errorMessage = L"GetAdaptersAddresses 失败。";
        return false;
    }

    for (PIP_ADAPTER_ADDRESSES adapter = pAddresses; adapter != NULL; adapter = adapter->Next) {
        if (adapter->OperStatus != IfOperStatusUp) {
            continue;
        }

        for (PIP_ADAPTER_GATEWAY_ADDRESS_LH gw = adapter->FirstGatewayAddress; gw != NULL; gw = gw->Next) {
            SOCKADDR* addr = gw->Address.lpSockaddr;
            if (!addr) {
                continue;
            }
            if (addr->sa_family == AF_INET) {
                char buf[INET_ADDRSTRLEN] = {0};
                sockaddr_in* in = reinterpret_cast<sockaddr_in*>(addr);
                InetNtopA(AF_INET, &in->sin_addr, buf, INET_ADDRSTRLEN);
                int wideSize = MultiByteToWideChar(CP_UTF8, 0, buf, -1, NULL, 0);
                if (wideSize > 0) {
                    gateway.resize(wideSize - 1);
                    MultiByteToWideChar(CP_UTF8, 0, buf, -1, gateway.data(), wideSize);
                    return true;
                }
            }
        }
    }

    errorMessage = L"未找到活动网关。";
    return false;
}

bool Network::IsHostReachable(const std::wstring& host, std::wstring& errorMessage, unsigned long timeoutMs) {
    errorMessage.clear();
    if (host.empty()) {
        errorMessage = L"主机地址为空。";
        return false;
    }

    // Convert wstring host to narrow string for getaddrinfo
    std::string narrowHost;
    int len = WideCharToMultiByte(CP_UTF8, 0, host.c_str(), static_cast<int>(host.size()), NULL, 0, NULL, NULL);
    if (len > 0) {
        narrowHost.resize(len);
        WideCharToMultiByte(CP_UTF8, 0, host.c_str(), static_cast<int>(host.size()), narrowHost.data(), len, NULL, NULL);
    } else {
        errorMessage = L"主机名编码转换失败。";
        return false;
    }

    struct addrinfo hints = {};
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_protocol = IPPROTO_TCP;

    struct addrinfo* result = nullptr;
    int rc = getaddrinfo(narrowHost.c_str(), "80", &hints, &result);
    if (rc != 0 || !result) {
        errorMessage = L"DNS 解析失败: " + host;
        return false;
    }

    SOCKET sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (sock == INVALID_SOCKET) {
        freeaddrinfo(result);
        errorMessage = L"创建 socket 失败。";
        return false;
    }

    // Set non-blocking for timeout
    u_long mode = 1;
    ioctlsocket(sock, FIONBIO, &mode);

    int connResult = connect(sock, result->ai_addr, static_cast<int>(result->ai_addrlen));
    freeaddrinfo(result);

    if (connResult == SOCKET_ERROR) {
        int err = WSAGetLastError();
        if (err != WSAEWOULDBLOCK) {
            closesocket(sock);
            errorMessage = L"连接失败。";
            return false;
        }
    }

    // Wait for connection with timeout
    fd_set writeSet;
    FD_ZERO(&writeSet);
    FD_SET(sock, &writeSet);

    struct timeval tv;
    tv.tv_sec = timeoutMs / 1000;
    tv.tv_usec = (timeoutMs % 1000) * 1000;

    int selectResult = select(0, NULL, &writeSet, NULL, &tv);
    closesocket(sock);

    if (selectResult <= 0) {
        errorMessage = L"连接超时: " + host;
        return false;
    }

    return true;
}
