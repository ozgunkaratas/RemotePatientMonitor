// Windows implementation (Win32 COM port API). The Linux one is serial_port.cpp.
#ifdef _WIN32

#include "serial_port.hpp"

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>

#include <algorithm>
#include <stdexcept>
#include <system_error>
#include <utility>

namespace rpm {

/// File-local helpers (a named namespace keeps Doxygen's call graphs correct).
namespace detail {

[[noreturn]] void throw_win32(DWORD error, const std::string& what)
{
    throw std::system_error(static_cast<int>(error), std::system_category(), what);
}

HANDLE as_handle(void* p) { return static_cast<HANDLE>(p); }

// "COM3" -> "\\.\COM3". The prefix is required from COM10 up and harmless below.
std::string device_name(const std::string& path)
{
    const std::string prefix = "\\\\.\\";
    return path.rfind(prefix, 0) == 0 ? path : prefix + path;
}

// Errors ReadFile reports when the USB serial adapter disappears.
bool is_disconnect(DWORD error)
{
    switch (error) {
    case ERROR_ACCESS_DENIED:
    case ERROR_BAD_COMMAND:
    case ERROR_DEVICE_NOT_CONNECTED:
    case ERROR_GEN_FAILURE:
    case ERROR_OPERATION_ABORTED:
    case ERROR_FILE_NOT_FOUND:
        return true;
    default:
        return false;
    }
}

} // namespace detail

SerialPort::SerialPort(const std::string& path, std::uint32_t baud) : path_(path)
{
    HANDLE h = ::CreateFileA(detail::device_name(path).c_str(), GENERIC_READ | GENERIC_WRITE,
                             0,  // exclusive access, like a tty
                             nullptr, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr);
    if (h == INVALID_HANDLE_VALUE) {
        detail::throw_win32(::GetLastError(), "open " + path);
    }
    handle_ = h;

    DCB dcb{};
    dcb.DCBlength = sizeof(dcb);
    if (!::GetCommState(h, &dcb)) {
        const DWORD err = ::GetLastError();
        close();
        detail::throw_win32(err, "GetCommState " + path);
    }

    dcb.BaudRate = baud;  // Windows accepts any rate the driver supports
    dcb.ByteSize = 8;
    dcb.Parity = NOPARITY;
    dcb.StopBits = ONESTOPBIT;
    dcb.fBinary = TRUE;
    dcb.fParity = FALSE;
    dcb.fOutxCtsFlow = FALSE;            // no hardware flow control
    dcb.fOutxDsrFlow = FALSE;
    dcb.fDsrSensitivity = FALSE;
    dcb.fOutX = FALSE;                   // no XON/XOFF
    dcb.fInX = FALSE;
    dcb.fNull = FALSE;
    dcb.fAbortOnError = FALSE;
    dcb.fDtrControl = DTR_CONTROL_ENABLE;  // raising DTR resets the Mega, as on Linux
    dcb.fRtsControl = RTS_CONTROL_ENABLE;

    if (!::SetCommState(h, &dcb)) {
        const DWORD err = ::GetLastError();
        close();
        detail::throw_win32(err, "SetCommState " + path + " (baud " + std::to_string(baud) + ")");
    }

    set_read_timeout(0);
}

SerialPort::~SerialPort() { close(); }

SerialPort::SerialPort(SerialPort&& other) noexcept
    : path_(std::move(other.path_)),
      handle_(std::exchange(other.handle_, nullptr)),
      timeout_ms_(std::exchange(other.timeout_ms_, -1))
{
}

SerialPort& SerialPort::operator=(SerialPort&& other) noexcept
{
    if (this != &other) {
        close();
        path_ = std::move(other.path_);
        handle_ = std::exchange(other.handle_, nullptr);
        timeout_ms_ = std::exchange(other.timeout_ms_, -1);
    }
    return *this;
}

void SerialPort::set_read_timeout(int timeout_ms)
{
    if (timeout_ms == timeout_ms_) {
        return;
    }
    COMMTIMEOUTS t{};
    if (timeout_ms <= 0) {
        // Return at once with whatever is already buffered.
        t.ReadIntervalTimeout = MAXDWORD;
        t.ReadTotalTimeoutMultiplier = 0;
        t.ReadTotalTimeoutConstant = 0;
    } else {
        // Return as soon as at least one byte is there, or after timeout_ms:
        // the Win32 equivalent of poll() followed by read().
        t.ReadIntervalTimeout = MAXDWORD;
        t.ReadTotalTimeoutMultiplier = MAXDWORD;
        t.ReadTotalTimeoutConstant = static_cast<DWORD>(timeout_ms);
    }
    t.WriteTotalTimeoutConstant = 1000;
    if (!::SetCommTimeouts(detail::as_handle(handle_), &t)) {
        detail::throw_win32(::GetLastError(), "SetCommTimeouts " + path_);
    }
    timeout_ms_ = timeout_ms;
}

std::size_t SerialPort::read_some(std::span<std::uint8_t> buffer, int timeout_ms)
{
    set_read_timeout(timeout_ms);

    const DWORD want = static_cast<DWORD>(std::min<std::size_t>(buffer.size(), MAXDWORD));
    DWORD got = 0;
    if (!::ReadFile(detail::as_handle(handle_), buffer.data(), want, &got, nullptr)) {
        const DWORD err = ::GetLastError();
        if (detail::is_disconnect(err)) {
            throw std::runtime_error(path_ + ": device disconnected");
        }
        detail::throw_win32(err, "ReadFile " + path_);
    }
    return static_cast<std::size_t>(got);  // 0 = timeout
}

void SerialPort::flush_input()
{
    ::PurgeComm(detail::as_handle(handle_), PURGE_RXCLEAR | PURGE_RXABORT);
}

void SerialPort::close() noexcept
{
    if (handle_ != nullptr) {
        ::CloseHandle(detail::as_handle(handle_));
        handle_ = nullptr;
    }
}

} // namespace rpm

#endif // _WIN32
