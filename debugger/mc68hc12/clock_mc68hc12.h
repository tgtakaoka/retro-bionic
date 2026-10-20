#ifndef __CLOCK_MC68HC12_H__
#define __CLOCK_MC68HC12_H__

#include <Arduino.h>

#include "pins_mc68hc12.h"

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

// The EXTAL level that moves E, found by sync_e().
extern uint8_t active_level;

inline void active_edge() {
    digitalWriteFast(PIN_EXTAL, active_level);
}

inline void quiet_edge() {
    digitalWriteFast(PIN_EXTAL, !active_level);
}

inline auto clock_e() {
    return digitalReadFast(PIN_E);
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

// Whether E reaches |level| soon after an edge.
bool wait_e(uint8_t level);
// Half an E cycle, when E runs.
void edge_pair();
// Finds which EXTAL level moves E and leaves the clock right after an E
// fall. Returns false when E doesn't move.
bool sync_e();

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
