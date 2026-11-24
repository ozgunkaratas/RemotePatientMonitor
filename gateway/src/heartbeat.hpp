#pragma once

#include <cstdint>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

namespace rpm {

// Phase 0 text heartbeat from the Mega: "HB seq=<n> uptime_ms=<ms> drop=<bytes>"
struct Heartbeat {
    std::uint32_t seq = 0;
    std::uint32_t uptime_ms = 0;
    std::uint32_t tx_dropped = 0;
};

// Returns nullopt for anything that is not a well-formed heartbeat line
// (half lines after a reset, noise, other messages).
std::optional<Heartbeat> parse_heartbeat(std::string_view line);

// Splits a byte stream into lines. Bytes may arrive in any chunking.
class LineAssembler {
public:
    explicit LineAssembler(std::size_t max_line = 256) : max_line_(max_line) {}

    // Feeds bytes and returns every line completed by them, without the '\n'
    // (a trailing '\r' is stripped too). Over-long lines are discarded.
    std::vector<std::string> feed(std::string_view bytes);

private:
    std::string partial_;
    std::size_t max_line_;
    bool discarding_ = false;
};

// Tracks heartbeat sequence numbers and classifies each new one.
class SequenceTracker {
public:
    enum class Event {
        First,     // first heartbeat seen
        InOrder,   // seq == previous + 1
        Gap,       // one or more heartbeats missing
        Reset,     // Mega restarted (seq back to 0)
        Duplicate, // same or older seq again
    };

    Event update(std::uint32_t seq);

    [[nodiscard]] std::uint64_t received() const noexcept { return received_; }
    [[nodiscard]] std::uint64_t lost() const noexcept { return lost_; }
    [[nodiscard]] std::uint64_t resets() const noexcept { return resets_; }
    [[nodiscard]] std::uint32_t last_gap() const noexcept { return last_gap_; }

private:
    std::optional<std::uint32_t> last_;
    std::uint64_t received_ = 0;
    std::uint64_t lost_ = 0;
    std::uint64_t resets_ = 0;
    std::uint32_t last_gap_ = 0;
};

} // namespace rpm
