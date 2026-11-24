cmake_minimum_required(VERSION 3.20)
project(rpm_gateway VERSION 0.1.0 LANGUAGES CXX)

option(RPM_GATEWAY_TESTS "Build the gateway unit tests" OFF)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
    set(CMAKE_BUILD_TYPE Release CACHE STRING "Build type" FORCE)
endif()

# Logic shared by the executable and the tests.
# Headers are listed so IDEs (Visual Studio, Qt Creator) show them in the project.
# Both serial port files are always listed; each compiles to nothing on the
# other platform (serial_port.cpp = Linux termios, serial_port_win32.cpp = Win32).
add_library(rpm_gateway_core STATIC
    src/heartbeat.hpp
    src/heartbeat.cpp
    src/serial_port.hpp
    src/serial_port.cpp
    src/serial_port_win32.cpp
)
target_include_directories(rpm_gateway_core PUBLIC src)
if(MSVC)
    target_compile_options(rpm_gateway_core PUBLIC /W4 /permissive- /utf-8)
else()
    target_compile_options(rpm_gateway_core PUBLIC -Wall -Wextra -Wpedantic -Wconversion -Wshadow)
endif()

if(WIN32)
    target_compile_definitions(rpm_gateway_core PUBLIC WIN32_LEAN_AND_MEAN NOMINMAX)
endif()

add_executable(rpm_gateway src/main.cpp)
target_link_libraries(rpm_gateway PRIVATE rpm_gateway_core)

if(RPM_GATEWAY_TESTS)
    enable_testing()
    add_executable(test_heartbeat tests/test_heartbeat.cpp)
    target_link_libraries(test_heartbeat PRIVATE rpm_gateway_core)
    add_test(NAME heartbeat COMMAND test_heartbeat)
endif()
