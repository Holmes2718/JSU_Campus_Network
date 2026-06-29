# DrCOM 校园网自动认证

Windows 原生 C++ 实现的 DrCOM 校园网自动认证工具，以系统托盘图标静默运行，检测到网络认证失效时自动重新登录。

**支持 WiFi 和有线连接。**

## 功能

- 自动检测是否进入校园网（WiFi SSID 匹配 / 有线服务器可达性检测）
- 每隔一定时间检测认证状态，认证失效自动重新登录
- 系统托盘右键菜单：立即认证、打开日志、重新加载配置、开机启动、退出
- 每日日志文件，保存在 `logs/` 目录
- 使用 `WaitableTimer` 低功耗等待，不占 CPU
- 纯 Windows API，无第三方依赖，内存占用极小

## 使用教程

### 1. 编译

**前置条件**：Visual Studio 2022（含 CMake 组件），或单独安装 CMake + MSVC。

```powershell
# 配置
cmake -S . -B build

# 编译 Release 版
cmake --build build --config Release
```

编译产物：`build\Release\DrcomAutoLogin.exe`

### 2. 配置

首次运行程序会自动在同目录生成 `config.ini`，修改其中的账号信息：

```ini
[Campus]
SSID=Campus-WiFi            # 学校 WiFi 名称（有线连接可忽略此项）
Gateway=192.168.254.17      # DrCOM 认证服务器地址（不要改）
Username=your-account@telecom   # 改成你的校园网账号
Password=your-password          # 改成你的密码
CheckInterval=10             # 在线时轮询间隔（秒），建议 10~30
AutoStart=0                  # 是否开机启动：0=否 1=是
```

| 配置项 | 说明 |
|--------|------|
| `SSID` | 学校 WiFi 名称，有线用户可忽略 |
| `Gateway` | DrCOM 认证服务器 IP，一般不需要改 |
| `Username` | 校园网账号 |
| `Password` | 校园网密码 |
| `CheckInterval` | 检查间隔（秒），太短会频繁请求 |
| `AutoStart` | 是否开机自启 |

修改配置后，右键托盘图标 → **重新读取配置** 即可生效，无需重启程序。

### 3. 运行

直接双击 `DrcomAutoLogin.exe`，程序会最小化到系统托盘。

右键托盘图标可操作：

| 菜单 | 功能 |
|------|------|
| 立即认证 | 立刻执行一次认证检测和登录 |
| 打开日志目录 | 打开 `logs/` 文件夹，查看运行日志 |
| 重新读取配置 | 修改 config.ini 后点此热加载 |
| 开机启动 | 勾选后每次开机自动运行 |
| 退出 | 关闭程序 |

### 4. 开机自启

右键托盘 → 勾选 **开机启动**。原理是将程序写入注册表 `HKCU\Software\Microsoft\Windows\CurrentVersion\Run`。勾选状态会同步写入 `config.ini`，下次启动仍然保持勾选。取消勾选即移除。

## 工作原理

```
┌──────────────┐     ┌─────────────────┐     ┌──────────────┐
│ 检测目标网络  │ ──→ │ 检测认证状态      │ ──→ │ 需要认证？    │
│ SSID / 可达性 │     │ GET 网关页面      │     │ 自动登录      │
└──────────────┘     └─────────────────┘     └──────────────┘
       ↑                                            │
       └──────── 等待 CheckInterval 秒 ──────────────┘
```

- **WiFi 连接**：检测当前 SSID 是否匹配配置
- **有线连接**：检测 DrCOM 认证服务器 `Gateway` 是否可达
- 两种连接方式均支持，满足其一即进入校园网检测流程

## 技术栈

- **语言**：C++17
- **GUI**：Win32 原生窗口 + 系统托盘
- **HTTP**：WinHTTP
- **WiFi 检测**：WLAN API
- **网络检测**：Winsock2 TCP Socket
- **定时器**：Windows Waitable Timer
- **编译**：CMake + MSVC

## 项目结构

```
├── CMakeLists.txt
├── README.md
└── src/
    ├── main.cpp        # 入口
    ├── config.h/cpp    # INI 配置读写
    ├── logger.h/cpp    # 日志（按日期分文件）
    ├── wifi.h/cpp      # WiFi SSID 检测
    ├── network.h/cpp   # 网关获取 / 主机可达性检测
    ├── http.h/cpp      # HTTP GET/HEAD 请求
    ├── login.h/cpp     # DrCOM 认证检测与登录
    ├── monitor.h/cpp   # 监控主循环
    ├── timer.h/cpp     # WaitableTimer 封装
    └── tray.h/cpp      # 系统托盘
```

## License

MIT
