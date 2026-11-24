#pragma once

#include <cstddef>
#include <cstdint>
#include <span>
#include <string>

namespace rpm {

// Raw 8N1 serial port. Owns the OS handle.
//   Linux:   serial_port.cpp        (termios), path like "/dev/mega" or "/dev/ttyACM0"
//   Windows: serial_port_win32.cpp  (Win32 COM API), path like "COM3"
class SerialPort {
public:
    SerialPort(const std::string& path, std::uint32_t baud);
    ~SerialPort();

    SerialPort(const SerialPort&) = delete;
    SerialPort& operator=(const SerialPort&) = delete;
    SerialPort(SerialPort&& other) noexcept;
    SerialPort& operator=(SerialPort&& other) noexcept;

    // Waits up to timeout_ms for data, then reads what is available.
    // Returns the number of bytes read, 0 on timeout. Throws on I/O errors
    // (for example when the Mega is unplugged).
    std::size_t read_some(std::span<std::uint8_t> buffer, int timeout_ms);

    // Drops anything already queued in the OS input buffer.
    void flush_input();

    [[nodiscard]] const std::string& path() const noexcept { return path_; }

private:
    void close() noexcept;

    std::string path_;
#ifdef _WIN32
    void set_read_timeout(int timeout_ms);

    void* handle_ = nullptr;  // HANDLE; nullptr when closed
    int timeout_ms_ = -1;     // timeout currently programmed into the port
#else
    int fd_ = -1;
#endif
};

} // namespace rpm
