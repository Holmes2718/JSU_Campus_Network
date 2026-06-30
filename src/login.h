#pragma once

#include "config.h"
#include "logger.h"
#include <string>

enum class AuthState {
    Unknown,
    Online,
    NeedAuth,
    Error,
};

struct AuthResult {
    AuthState state = AuthState::Unknown;
    std::wstring message;
};

class LoginService {
public:
    // Perform login, or detect online status if already logged in.
    // Returns Online ("已经在线" / "登录成功") or Error ("登录失败").
    static AuthResult PerformLogin(const Config& config, Logger& logger);
};
