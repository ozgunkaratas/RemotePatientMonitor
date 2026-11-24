// Unit tests for the phase 0 heartbeat parsing. No framework yet: each CHECK
// prints the failing expression and the test exits non-zero.
// Verifies: SYS-021 (gap detection), SYS-023 (ignore malformed lines).

#include <cstdio>
#include <string>

#include "heartbeat.hpp"

/// Unit tests for heartbeat.hpp (a named namespace keeps Doxygen's call graphs correct).
namespace rpm::tests {

int g_failures = 0;

#define CHECK(expr)                                                              \
    do {                                                                         \
        if (!(expr)) {                                                           \
            std::fprintf(stderr, "%s:%d: CHECK failed: %s\n", __FILE__, __LINE__, \
                         #expr);                                                 \
            ++g_failures;                                                        \
        }                                                                        \
    } while (0)

void parse_valid_line()
{
    const auto hb = rpm::parse_heartbeat("HB seq=42 uptime_ms=4200 drop=0");
    CHECK(hb.has_value());
    CHECK(hb->seq == 42U);
    CHECK(hb->uptime_ms == 4200U);
    CHECK(hb->tx_dropped == 0U);
}

void parse_rejects_malformed_lines()
{
    CHECK(!rpm::parse_heartbeat(""));
    CHECK(!rpm::parse_heartbeat("HB"));
    CHECK(!rpm::parse_heartbeat("seq=1 uptime_ms=2 drop=0"));         // no prefix
    CHECK(!rpm::parse_heartbeat("HB seq=1 uptime_ms=2"));             // missing field
    CHECK(!rpm::parse_heartbeat("HB seq=x uptime_ms=2 drop=0"));      // not a number
    CHECK(!rpm::parse_heartbeat("HB seq=1 uptime_ms=2 drop=0 junk")); // trailing data
    CHECK(!rpm::parse_heartbeat("HB seq=99999999999 uptime_ms=2 drop=0")); // overflow
    CHECK(!rpm::parse_heartbeat("me=12 uptime_ms=4200 drop=0"));      // half line after reset
}

void assembler_handles_any_chunking()
{
    rpm::LineAssembler a;
    CHECK(a.feed("HB seq=1 up").empty());
    const auto lines = a.feed("time_ms=100 drop=0\r\nHB seq=2 uptime_ms=200 drop=0\nHB");
    CHECK(lines.size() == 2U);
    CHECK(lines.at(0) == "HB seq=1 uptime_ms=100 drop=0");
    CHECK(lines.at(1) == "HB seq=2 uptime_ms=200 drop=0");
    const auto rest = a.feed(" seq=3 uptime_ms=300 drop=0\n");
    CHECK(rest.size() == 1U && rest.at(0) == "HB seq=3 uptime_ms=300 drop=0");
}

void assembler_discards_overlong_lines()
{
    rpm::LineAssembler a(8);
    const auto lines = a.feed("0123456789ABCDEF\nshort\n");
    CHECK(lines.size() == 1U);
    CHECK(lines.at(0) == "short");
}

void tracker_classifies_sequences()
{
    using Ev = rpm::SequenceTracker::Event;
    rpm::SequenceTracker t;
    CHECK(t.update(5) == Ev::First);
    CHECK(t.update(6) == Ev::InOrder);
    CHECK(t.update(9) == Ev::Gap);
    CHECK(t.last_gap() == 2U);
    CHECK(t.update(9) == Ev::Duplicate);
    CHECK(t.update(7) == Ev::Duplicate);
    CHECK(t.update(10) == Ev::InOrder);  // duplicates did not rewind the tracker
    CHECK(t.update(0) == Ev::Reset);
    CHECK(t.update(1) == Ev::InOrder);
    CHECK(t.received() == 8U);
    CHECK(t.lost() == 2U);
    CHECK(t.resets() == 1U);
}

} // namespace rpm::tests

int main()
{
    rpm::tests::parse_valid_line();
    rpm::tests::parse_rejects_malformed_lines();
    rpm::tests::assembler_handles_any_chunking();
    rpm::tests::assembler_discards_overlong_lines();
    rpm::tests::tracker_classifies_sequences();

    if (rpm::tests::g_failures != 0) {
        std::fprintf(stderr, "%d check(s) failed\n", rpm::tests::g_failures);
        return 1;
    }
    std::printf("all heartbeat tests passed\n");
    return 0;
}
