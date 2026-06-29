cmake_minimum_required(VERSION 3.18)
project(DrcomAutoLogin VERSION 1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

if (WIN32)
    add_definitions(-DUNICODE -D_UNICODE)
endif()

add_executable(DrcomAutoLogin WIN32
    src/main.cpp
    src/config.cpp
    src/logger.cpp
    src/wifi.cpp
    src/network.cpp
    src/http.cpp
    src/login.cpp
    src/monitor.cpp
    src/tray.cpp
    src/timer.cpp
)

target_include_directories(DrcomAutoLogin PRIVATE src)

target_compile_options(DrcomAutoLogin PRIVATE /utf-8)

target_link_options(DrcomAutoLogin PRIVATE /SUBSYSTEM:WINDOWS)

target_link_libraries(DrcomAutoLogin PRIVATE
    winhttp
    wlanapi
    ole32
    shell32
    user32
    advapi32
)
