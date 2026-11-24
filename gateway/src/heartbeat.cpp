#include "heartbeat.hpp"

#include <charconv>

namespace rpm {

/// File-local helpers (a named namespace keeps Doxygen's call graphs correct).
namespace detail {

/**
 * @brief Parses one "<key>=<value>" field from the front of @p rest.
 *
 * Consumes the field and the space after it, so repeated calls walk the line.
 *
 * @par Example
 * @code
 * std::string_view rest = "seq=42 uptime_ms=4200";
 * take_field(rest, "seq", out);  // true, out == 42, rest == "uptime_ms=4200"
 * @endcode
 *
 * @param[in,out] rest  Remaining line; advanced past the field on success.
 * @param[in]     key   Expected field name, without '='.
 * @param[out]    out   Parsed value; written only on success.
 * @return true if the field was parsed, false if it is malformed.
 */
bool take_field(std::string_view& rest, std::string_view key, std::uint32_t& out)
{
    if (!rest.starts_with(key) || rest.size() <= key.size() || rest[key.size()] != '=') {
        return false;
    }
    rest.remove_prefix(key.size() + 1);

    const char* first = rest.data();
    const char* last = rest.data() + rest.size();
    const auto [ptr, ec] = std::from_chars(first, last, out);
    if (ec != std::errc{} || ptr == first) {
        return false;
    }
    rest.remove_prefix(static_cast<std::size_t>(ptr - first));
    if (!rest.empty()) {
        if (rest.front() != ' ') {
            return false;
        }
        rest.remove_prefix(1);
    }
    return true;
}

} // namespace detail

std::optional<Heartbeat> parse_heartbeat(std::string_view line)
{
    constexpr std::string_view prefix = "HB ";
    if (!line.starts_with(prefix)) {
        return std::nullopt;
    }
    line.remove_prefix(prefix.size());

    Heartbeat hb;
    if (!detail::take_field(line, "seq", hb.seq) ||
        !detail::take_field(line, "uptime_ms", hb.uptime_ms) ||
        !detail::take_field(line, "drop", hb.tx_dropped) || !line.empty()) {
        return std::nullopt;
    }
    return hb;
}

std::vector<std::string> LineAssembler::feed(std::string_view bytes)
{
    std::vector<std::string> lines;
    for (const char c : bytes) {
        if (c == '\n') {
            if (!discarding_) {
                if (!partial_.empty() && partial_.back() == '\r') {
                    partial_.pop_back();
                }
                lines.push_back(std::move(partial_));
            }
            partial_.clear();
            discarding_ = false;
            continue;
        }
        if (discarding_) {
            continue;
        }
        if (partial_.size() >= max_line_) {
            partial_.clear();
            discarding_ = true;  // drop the rest of this line, resync at the next '\n'
            continue;
        }
        partial_.push_back(c);
    }
    return lines;
}

SequenceTracker::Event SequenceTracker::update(std::uint32_t seq)
{
    ++received_;
    last_gap_ = 0;

    if (!last_) {
        last_ = seq;
        return Event::First;
    }

    const std::uint32_t expected = *last_ + 1U;
    if (seq == expected) {
        last_ = seq;
        return Event::InOrder;
    }
    if (seq == 0U) {
        ++resets_;
        last_ = seq;
        return Event::Reset;
    }
    if (seq > expected) {
        last_gap_ = seq - expected;
        lost_ += last_gap_;
        last_ = seq;
        return Event::Gap;
    }
    return Event::Duplicate;  // keep last_: an old frame must not rewind the tracker
}

} // namespace rpm
