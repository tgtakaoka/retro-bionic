#ifndef __CLOCK_MC68HC12_H__
#define __CLOCK_MC68HC12_H__

#include <Arduino.h>

// The board's pins_*.h, included before this, names PIN_EXTAL and PIN_E.

namespace debugger {
namespace mc68hc12 {

// The EXTAL edges the debugger drives and the E clock it watches, shared by
// the members of the family. A bus cycle is four EXTAL edges: E moves on
// the active ones, the quiet ones fall between them.
namespace clock {

constexpr auto extal_ns = 25;    // an EXTAL half period, at least
constexpr auto addr_ns = 40;     // quiet edge to address, R/W, #LSTRB
constexpr auto dbe_ns = 40;      // quiet edge to #DBE, write data
constexpr auto setup_ns = 25;    // read data to the E fall
constexpr auto e_wait_ns = 200;  // active edge to E, at most
constexpr uint32_t e_wait_cycles = F_CPU / 1000000 * e_wait_ns / 1000;

// The clock on a board's EXTAL and E pins.
template <uint8_t EXTAL, uint8_t E>
struct Edges {
    // The EXTAL level that moves E, found by sync_e().
    static inline uint8_t active_level = HIGH;

    static void active_edge() { digitalWriteFast(EXTAL, active_level); }
    static void quiet_edge() { digitalWriteFast(EXTAL, !active_level); }
    static auto clock_e() { return digitalReadFast(E); }

    // Whether E reaches |level| soon after an edge.
    static bool wait_e(uint8_t level) {
        const uint32_t start = ARM_DWT_CYCCNT;
        do {
            if (clock_e() == level)
                return true;
        } while (ARM_DWT_CYCCNT - start < e_wait_cycles);
        return false;
    }

    // Half an E cycle, when E runs.
    static void edge_pair() {
        quiet_edge();
        delayNanoseconds(extal_ns);
        const auto e = clock_e();
        active_edge();
        wait_e(!e);
    }

    // Finds which EXTAL level moves E and leaves the clock right after an E
    // fall. Returns false when E doesn't move.
    static bool sync_e() {
        auto level = digitalReadFast(EXTAL);
        auto e = clock_e();
        for (auto n = 0; n < 16; ++n) {
            level = !level;
            digitalWriteFast(EXTAL, level);
            if (wait_e(!e)) {
                active_level = level;
                for (auto i = 0; clock_e() != LOW && i < 8; ++i)
                    edge_pair();
                return clock_e() == LOW;
            }
        }
        return false;
    }
};

// This board's clock, for the file that includes it.
namespace {
using Board = Edges<PIN_EXTAL, PIN_E>;

inline void active_edge() {
    Board::active_edge();
}

inline void quiet_edge() {
    Board::quiet_edge();
}

inline auto clock_e() {
    return Board::clock_e();
}

inline bool wait_e(uint8_t level) {
    return Board::wait_e(level);
}

inline void edge_pair() {
    Board::edge_pair();
}

inline bool sync_e() {
    return Board::sync_e();
}

inline auto reset_signal() {
    return digitalReadFast(PIN_RESET);
}

inline void assert_reset() {
    digitalWriteFast(PIN_RESET, LOW);
    pinMode(PIN_RESET, OUTPUT_OPENDRAIN);
}

inline void negate_reset() {
    pinMode(PIN_RESET, INPUT_PULLUP);
}
}  // namespace

}  // namespace clock
}  // namespace mc68hc12
}  // namespace debugger
#endif

// Local Variables:
// mode: c++
// c-basic-offset: 4
// tab-width: 4
// End:
// vim: set ft=cpp et ts=4 sw=4:
