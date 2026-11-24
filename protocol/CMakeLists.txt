cmake_minimum_required(VERSION 3.20)
project(rpm_protocol VERSION 0.1.0 LANGUAGES C)

option(RPM_PROTOCOL_TESTS "Build the protocol unit tests" OFF)

# The header is listed so IDEs (Visual Studio, Qt Creator) show it in the project.
add_library(rpm_protocol STATIC
    include/rpm/protocol.h
    src/crc16.c
    src/cobs.c
)
target_include_directories(rpm_protocol PUBLIC include)
set_target_properties(rpm_protocol PROPERTIES C_STANDARD 11 C_STANDARD_REQUIRED ON)
if(MSVC)
    target_compile_options(rpm_protocol PRIVATE /W4)
else()
    target_compile_options(rpm_protocol PRIVATE -Wall -Wextra -Wpedantic -Wconversion -Wshadow)
endif()

if(RPM_PROTOCOL_TESTS)
    enable_testing()
    add_executable(test_protocol tests/test_protocol.c)
    target_link_libraries(test_protocol PRIVATE rpm_protocol)
    # Address + undefined-behaviour sanitizers where the toolchain has them (GCC/Clang on Linux)
    if(CMAKE_C_COMPILER_ID MATCHES "GNU|Clang" AND CMAKE_SYSTEM_NAME STREQUAL "Linux")
        target_compile_options(test_protocol PRIVATE -fsanitize=address,undefined)
        target_link_options(test_protocol PRIVATE -fsanitize=address,undefined)
    endif()
    add_test(NAME protocol COMMAND test_protocol)
endif()
