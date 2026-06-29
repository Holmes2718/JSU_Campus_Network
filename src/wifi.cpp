#include "wifi.h"
#include <windows.h>
#include <wlanapi.h>
#include <objbase.h>
#include <wtypes.h>
#include <string>

#pragma comment(lib, "Wlanapi.lib")
#pragma comment(lib, "Ole32.lib")

static std::wstring ConvertSsid(const DOT11_SSID& ssid) {
    if (ssid.uSSIDLength == 0) {
        return L"";
    }
    std::string raw(reinterpret_cast<const char*>(ssid.ucSSID), ssid.uSSIDLength);
    int wideSize = MultiByteToWideChar(CP_UTF8, 0, raw.c_str(), static_cast<int>(raw.size()), NULL, 0);
    if (wideSize <= 0) {
        wideSize = MultiByteToWideChar(CP_ACP, 0, raw.c_str(), static_cast<int>(raw.size()), NULL, 0);
    }
    if (wideSize <= 0) {
        return L"";
    }
    std::wstring result;
    result.resize(wideSize);
    if (MultiByteToWideChar(CP_UTF8, 0, raw.c_str(), static_cast<int>(raw.size()), result.data(), wideSize) <= 0) {
        MultiByteToWideChar(CP_ACP, 0, raw.c_str(), static_cast<int>(raw.size()), result.data(), wideSize);
    }
    return result;
}

bool Wifi::GetCurrentSsid(std::wstring& ssid, std::wstring& errorMessage) {
    ssid.clear();
    errorMessage.clear();

    DWORD negotiatedVersion = 0;
    HANDLE clientHandle = NULL;
    DWORD result = WlanOpenHandle(2, NULL, &negotiatedVersion, &clientHandle);
    if (result != ERROR_SUCCESS) {
        errorMessage = L"WlanOpenHandle 失败。";
        return false;
    }

    PWLAN_INTERFACE_INFO_LIST interfaceList = NULL;
    result = WlanEnumInterfaces(clientHandle, NULL, &interfaceList);
    if (result != ERROR_SUCCESS || interfaceList == NULL) {
        errorMessage = L"WlanEnumInterfaces 失败。";
        if (interfaceList) {
            WlanFreeMemory(interfaceList);
        }
        WlanCloseHandle(clientHandle, NULL);
        return false;
    }

    bool found = false;
    for (unsigned int i = 0; i < interfaceList->dwNumberOfItems; ++i) {
        const WLAN_INTERFACE_INFO& interfaceInfo = interfaceList->InterfaceInfo[i];
        if (interfaceInfo.isState != wlan_interface_state_connected) {
            continue;
        }

        PWLAN_CONNECTION_ATTRIBUTES connectionAttributes = NULL;
        DWORD dataSize = 0;
        WLAN_OPCODE_VALUE_TYPE opCode = wlan_opcode_value_type_invalid;
        result = WlanQueryInterface(clientHandle,
            &interfaceInfo.InterfaceGuid,
            wlan_intf_opcode_current_connection,
            NULL,
            &dataSize,
            reinterpret_cast<PVOID*>(&connectionAttributes),
            &opCode);

        if (result == ERROR_SUCCESS && connectionAttributes != NULL) {
            ssid = ConvertSsid(connectionAttributes->wlanAssociationAttributes.dot11Ssid);
            WlanFreeMemory(connectionAttributes);
            found = true;
            break;
        }

        if (connectionAttributes) {
            WlanFreeMemory(connectionAttributes);
        }
    }

    WlanFreeMemory(interfaceList);
    WlanCloseHandle(clientHandle, NULL);

    if (!found) {
        errorMessage = L"未连接到 WiFi。";
        return false;
    }

    return true;
}
