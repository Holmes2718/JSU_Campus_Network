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
    static AuthResult DetectAuthStatus(const Config& config, Logger& logger);
    static AuthResult PerformLogin(const Config& config, Logger& logger);
};
