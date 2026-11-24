// rpm_gateway — phase 0
//
// Opens the Mega's serial port, waits for the first heartbeat (the Mega resets
// when the port opens), then prints every heartbeat with the gateway's own
// timestamp and warns about lost or repeated ones. Ctrl-C prints a summary.
//
// Usage: rpm_gateway [device] [baud]
//   defaults: /dev/mega 115200 on Linux, COM3 115200 on Windows

#include <array>
#include <atomic>
#include <chrono>
#include <csignal>
#include <cstdint>
#include <cstdio>
#include <exception>
#include <string>
#include <string_view>

#include "heartbeat.hpp"
#include "serial_port.hpp"

/// Helpers of the rpm_gateway program (a named namespace keeps Doxygen's call graphs correct).
namespace rpm::app {

std::atomic<bool> g_stop{false};

extern "C" void on_signal(int) { g_stop.store(true); }

#ifdef _WIN32
constexpr const char* kDefaultDevice = "COM3";  // see Device Manager > Ports (COM & LPT)

void install_signal_handlers()
{
    // The CRT maps Ctrl-C and Ctrl-Break to SIGINT. The read loop wakes up at
    // least every 100 ms, so the flag is noticed quickly.
    std::signal(SIGINT, on_signal);
    std::signal(SIGTERM, on_signal);
}
#else
constexpr const char* kDefaultDevice = "/dev/mega";

void install_signal_handlers()
{
    struct sigaction sa{};
    sa.sa_handler = on_signal;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = 0;  // no SA_RESTART: poll() returns EINTR so we notice Ctrl-C at once
    sigaction(SIGINT, &sa, nullptr);
    sigaction(SIGTERM, &sa, nullptr);
}
#endif

double seconds_since(std::chrono::steady_clock::time_point start)
{
    return std::chrono::duration<double>(std::chrono::steady_clock::now() - start).count();
}

constexpr int kHeartbeatTimeoutMs = 1000;  // SYS-022: link lost after 1 s of silence

} // namespace rpm::app

int main(int argc, char** argv)
{
    const std::string device = argc > 1 ? argv[1] : rpm::app::kDefaultDevice;
    const std::uint32_t baud = argc > 2 ? static_cast<std::uint32_t>(std::stoul(argv[2])) : 115200U;

    rpm::app::install_signal_handlers();

    try {
        rpm::SerialPort port(device, baud);
        std::printf("[rpm_gateway] opened %s at %u baud\n", device.c_str(), baud);
        std::printf("[rpm_gateway] waiting for the first heartbeat "
                    "(the Mega resets when the port opens)\n");
        port.flush_input();

        const auto start = std::chrono::steady_clock::now();
        auto last_rx = start;
        bool link_up = false;
        bool link_lost_reported = false;

        rpm::LineAssembler assembler;
        rpm::SequenceTracker tracker;
        std::array<std::uint8_t, 512> buf{};

        while (!rpm::app::g_stop.load()) {
            const std::size_t n = port.read_some(buf, 100);

            for (const auto& line :
                 assembler.feed(std::string_view(reinterpret_cast<const char*>(buf.data()), n))) {
                const auto hb = rpm::parse_heartbeat(line);
                if (!hb) {
                    // SYS-023: ignore anything before the first valid heartbeat.
                    if (link_up) {
                        std::printf("[%9.3f s] ?? %s\n", rpm::app::seconds_since(start),
                                    line.c_str());
                    }
                    continue;
                }

                last_rx = std::chrono::steady_clock::now();
                if (link_lost_reported) {
                    std::printf("[%9.3f s] link restored\n", rpm::app::seconds_since(start));
                    link_lost_reported = false;
                }
                link_up = true;

                using Ev = rpm::SequenceTracker::Event;
                switch (tracker.update(hb->seq)) {
                case Ev::Gap:
                    std::printf("[%9.3f s] WARNING: %u heartbeat(s) lost before seq=%u\n",
                                rpm::app::seconds_since(start), tracker.last_gap(), hb->seq);
                    break;
                case Ev::Reset:
                    std::printf("[%9.3f s] WARNING: Mega restarted (seq back to 0)\n",
                                rpm::app::seconds_since(start));
                    break;
                case Ev::Duplicate:
                    std::printf("[%9.3f s] WARNING: duplicate or old seq=%u\n",
                                rpm::app::seconds_since(start), hb->seq);
                    break;
                case Ev::First:
                case Ev::InOrder:
                    break;
                }
                std::printf("[%9.3f s] %s\n", rpm::app::seconds_since(start), line.c_str());
            }

            const auto silence = std::chrono::duration_cast<std::chrono::milliseconds>(
                std::chrono::steady_clock::now() - last_rx);
            if (link_up && !link_lost_reported && silence.count() > rpm::app::kHeartbeatTimeoutMs) {
                std::printf("[%9.3f s] WARNING: no heartbeat for %lld ms (link lost)\n",
                            rpm::app::seconds_since(start),
                            static_cast<long long>(silence.count()));
                link_lost_reported = true;
            }
            std::fflush(stdout);
        }

        std::printf("\n[rpm_gateway] summary after %.1f s: received=%llu lost=%llu resets=%llu\n",
                    rpm::app::seconds_since(start),
                    static_cast<unsigned long long>(tracker.received()),
                    static_cast<unsigned long long>(tracker.lost()),
                    static_cast<unsigned long long>(tracker.resets()));
        return 0;
    } catch (const std::exception& e) {
        std::fprintf(stderr, "[rpm_gateway] error: %s\n", e.what());
        return 1;
    }
}
