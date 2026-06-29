#include "tray.h"
#include <windows.h>
#include <shellapi.h>
#include <shlobj.h>
#include <string>

static const UINT WM_TRAYICON = WM_APP + 1;
static const UINT ID_TRAY_IMMEDIATE = 1001;
static const UINT ID_TRAY_OPEN_LOGS = 1002;
static const UINT ID_TRAY_RELOAD = 1003;
static const UINT ID_TRAY_EXIT = 1004;
static const UINT ID_TRAY_AUTOSTART = 1005;

Tray::Tray(const Config& config,
           const Logger& logger,
           const std::wstring& configPath,
           std::function<void()> immediateAuth,
           std::function<void()> openLogs,
           std::function<void()> reloadConfig,
           std::function<void()> exitApp)
    : m_instance(GetModuleHandleW(NULL)),
      m_window(NULL),
      m_configPath(configPath),
      m_appName(L"DrCOM 自动认证"),
      m_autoStart(config.autoStart),
      m_logger(logger),
      m_immediateAuth(std::move(immediateAuth)),
      m_openLogs(std::move(openLogs)),
      m_reloadConfig(std::move(reloadConfig)),
      m_exitApp(std::move(exitApp)) {
    CreateWindowClass();
    CreateTrayIcon();
}

Tray::~Tray() {
    RemoveTrayIcon();
    if (m_window) {
        DestroyWindow(m_window);
        m_window = NULL;
    }
}

void Tray::CreateWindowClass() {
    WNDCLASSW wc = {};
    wc.lpfnWndProc = Tray::WindowProc;
    wc.hInstance = m_instance;
    wc.lpszClassName = L"DrcomAutoLoginTrayClass";
    wc.hIcon = LoadIconW(NULL, IDI_APPLICATION);
    wc.cbClsExtra = 0;
    wc.cbWndExtra = 0;

    RegisterClassW(&wc);
    m_window = CreateWindowW(wc.lpszClassName, m_appName.c_str(), WS_OVERLAPPEDWINDOW,
                             CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT,
                             NULL, NULL, m_instance, this);
}

void Tray::CreateTrayIcon() {
    if (!m_window) {
        return;
    }

    NOTIFYICONDATAW nid = {};
    nid.cbSize = sizeof(nid);
    nid.hWnd = m_window;
    nid.uID = 1;
    nid.uFlags = NIF_MESSAGE | NIF_ICON | NIF_TIP;
    nid.uCallbackMessage = WM_TRAYICON;
    nid.hIcon = LoadIconW(NULL, IDI_APPLICATION);
    wcscpy_s(nid.szTip, L"校园网自动认证 (WiFi/有线)");
    Shell_NotifyIconW(NIM_ADD, &nid);
}

void Tray::RemoveTrayIcon() {
    if (!m_window) {
        return;
    }
    NOTIFYICONDATAW nid = {};
    nid.cbSize = sizeof(nid);
    nid.hWnd = m_window;
    nid.uID = 1;
    Shell_NotifyIconW(NIM_DELETE, &nid);
}

void Tray::ShowContextMenu(HWND hwnd) {
    POINT pt;
    GetCursorPos(&pt);

    HMENU menu = CreatePopupMenu();
    AppendMenuW(menu, MF_STRING, ID_TRAY_IMMEDIATE, L"立即认证");
    AppendMenuW(menu, MF_STRING, ID_TRAY_OPEN_LOGS, L"打开日志目录");
    AppendMenuW(menu, MF_STRING, ID_TRAY_RELOAD, L"重新读取配置");
    AppendMenuW(menu, MF_SEPARATOR, 0, NULL);
    AppendMenuW(menu, MF_STRING | (m_autoStart ? MF_CHECKED : MF_UNCHECKED), ID_TRAY_AUTOSTART, L"开机启动");
    AppendMenuW(menu, MF_SEPARATOR, 0, NULL);
    AppendMenuW(menu, MF_STRING, ID_TRAY_EXIT, L"退出");

    SetForegroundWindow(hwnd);
    TrackPopupMenu(menu, TPM_RIGHTBUTTON, pt.x, pt.y, 0, hwnd, NULL);
    DestroyMenu(menu);
}

void Tray::Run() {
    if (!m_window) {
        return;
    }

    ShowWindow(m_window, SW_HIDE);
    MSG msg;
    while (GetMessageW(&msg, NULL, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }
}

void Tray::ApplyAutoStart(bool enable) {
    // Write registry (actual auto-start mechanism)
    HKEY key = NULL;
    LONG result = RegOpenKeyExW(HKEY_CURRENT_USER, L"Software\\Microsoft\\Windows\\CurrentVersion\\Run", 0, KEY_WRITE, &key);
    if (result == ERROR_SUCCESS && key) {
        if (enable) {
            wchar_t exePath[MAX_PATH] = {};
            GetModuleFileNameW(NULL, exePath, MAX_PATH);
            RegSetValueExW(key, L"DrcomAutoLogin", 0, REG_SZ, reinterpret_cast<const BYTE*>(exePath), static_cast<DWORD>((wcslen(exePath) + 1) * sizeof(wchar_t)));
        } else {
            RegDeleteValueW(key, L"DrcomAutoLogin");
        }
        RegCloseKey(key);
    }

    // Sync config.ini so the checkbox state persists across restarts
    WritePrivateProfileStringW(L"Campus", L"AutoStart", enable ? L"1" : L"0", m_configPath.c_str());
}

LRESULT CALLBACK Tray::WindowProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam) {
    Tray* tray = nullptr;
    if (message == WM_CREATE) {
        LPCREATESTRUCTW cs = reinterpret_cast<LPCREATESTRUCTW>(lParam);
        tray = reinterpret_cast<Tray*>(cs->lpCreateParams);
        SetWindowLongPtrW(hwnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(tray));
    } else {
        tray = reinterpret_cast<Tray*>(GetWindowLongPtrW(hwnd, GWLP_USERDATA));
    }

    if (!tray) {
        return DefWindowProcW(hwnd, message, wParam, lParam);
    }

    switch (message) {
    case WM_TRAYICON:
        if (lParam == WM_RBUTTONUP) {
            tray->ShowContextMenu(hwnd);
        }
        break;
    case WM_COMMAND:
        switch (LOWORD(wParam)) {
        case ID_TRAY_IMMEDIATE:
            tray->m_immediateAuth();
            break;
        case ID_TRAY_OPEN_LOGS:
            tray->m_openLogs();
            break;
        case ID_TRAY_RELOAD:
            tray->m_reloadConfig();
            break;
        case ID_TRAY_AUTOSTART:
            tray->m_autoStart = !tray->m_autoStart;
            tray->ApplyAutoStart(tray->m_autoStart);
            break;
        case ID_TRAY_EXIT:
            PostQuitMessage(0);
            tray->m_exitApp();
            break;
        }
        break;
    case WM_DESTROY:
        PostQuitMessage(0);
        break;
    default:
        return DefWindowProcW(hwnd, message, wParam, lParam);
    }

    return 0;
}
