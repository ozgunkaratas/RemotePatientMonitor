// Linux / POSIX implementation (termios). The Windows one is serial_port_win32.cpp.
#ifndef _WIN32

#include "serial_port.hpp"

#include <cerrno>
#include <cstring>
#include <stdexcept>
#include <system_error>
#include <utility>

#include <fcntl.h>
#include <poll.h>
#include <termios.h>
#include <unistd.h>

namespace rpm {

/// File-local helpers (a named namespace keeps Doxygen's call graphs correct).
namespace detail {

[[noreturn]] void throw_errno(const std::string& what)
{
    throw std::system_error(errno, std::generic_category(), what);
}

speed_t to_speed(std::uint32_t baud)
{
    switch (baud) {
    case 9600: return B9600;
    case 19200: return B19200;
    case 38400: return B38400;
    case 57600: return B57600;
    case 115200: return B115200;
    case 230400: return B230400;
#ifdef B500000
    case 500000: return B500000;
#endif
#ifdef B1000000
    case 1000000: return B1000000;
#endif
    default: throw std::invalid_argument("unsupported baud rate: " + std::to_string(baud));
    }
}

} // namespace detail

SerialPort::SerialPort(const std::string& path, std::uint32_t baud) : path_(path)
{
    fd_ = ::open(path.c_str(), O_RDWR | O_NOCTTY | O_CLOEXEC);
    if (fd_ < 0) {
        detail::throw_errno("open " + path);
    }

    termios tio{};
    if (::tcgetattr(fd_, &tio) != 0) {
        const int err = errno;
        close();
        errno = err;
        detail::throw_errno("tcgetattr " + path);
    }

    ::cfmakeraw(&tio);              // no line editing, no echo, no CR/LF translation
    tio.c_cflag |= CLOCAL | CREAD;  // ignore modem lines, enable receiver
    tio.c_cflag &= ~CRTSCTS;        // no hardware flow control
    tio.c_cc[VMIN] = 0;             // poll() does the waiting
    tio.c_cc[VTIME] = 0;

    const speed_t speed = detail::to_speed(baud);
    ::cfsetispeed(&tio, speed);
    ::cfsetospeed(&tio, speed);

    if (::tcsetattr(fd_, TCSANOW, &tio) != 0) {
        const int err = errno;
        close();
        errno = err;
        detail::throw_errno("tcsetattr " + path);
    }
}

SerialPort::~SerialPort() { close(); }

SerialPort::SerialPort(SerialPort&& other) noexcept
    : path_(std::move(other.path_)), fd_(std::exchange(other.fd_, -1))
{
}

SerialPort& SerialPort::operator=(SerialPort&& other) noexcept
{
    if (this != &other) {
        close();
        path_ = std::move(other.path_);
        fd_ = std::exchange(other.fd_, -1);
    }
    return *this;
}

std::size_t SerialPort::read_some(std::span<std::uint8_t> buffer, int timeout_ms)
{
    pollfd pfd{fd_, POLLIN, 0};
    const int ready = ::poll(&pfd, 1, timeout_ms);
    if (ready < 0) {
        if (errno == EINTR) {
            return 0;  // a signal (e.g. Ctrl-C) woke us; let the caller check its flag
        }
        detail::throw_errno("poll " + path_);
    }
    if (ready == 0) {
        return 0;
    }
    if ((pfd.revents & (POLLERR | POLLHUP | POLLNVAL)) != 0) {
        throw std::runtime_error(path_ + ": device disconnected");
    }

    const ssize_t n = ::read(fd_, buffer.data(), buffer.size());
    if (n < 0) {
        if (errno == EINTR || errno == EAGAIN) {
            return 0;
        }
        detail::throw_errno("read " + path_);
    }
    if (n == 0) {
        throw std::runtime_error(path_ + ": device disconnected");
    }
    return static_cast<std::size_t>(n);
}

void SerialPort::flush_input() { ::tcflush(fd_, TCIFLUSH); }

void SerialPort::close() noexcept
{
    if (fd_ >= 0) {
        ::close(fd_);
        fd_ = -1;
    }
}

} // namespace rpm

#endif // !_WIN32
