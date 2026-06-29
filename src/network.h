#pragma once

#include <string>

class Network {
public:
    // Retrieves the IPv4 default gateway of the active adapter (as dotted string).
    // Returns true on success and fills `gateway`; otherwise returns false and sets `errorMessage`.
    static bool GetDefaultGateway(std::wstring& gateway, std::wstring& errorMessage);

    // Checks if a host is reachable via TCP on port 80.
    // Returns true if connection succeeds within timeoutMs; otherwise false and sets `errorMessage`.
    static bool IsHostReachable(const std::wstring& host, std::wstring& errorMessage, unsigned long timeoutMs = 2000);
};
