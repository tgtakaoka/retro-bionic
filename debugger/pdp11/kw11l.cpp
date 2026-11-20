#include "kw11l.h"
#include "debugger.h"

namespace debugger {
namespace pdp11 {

namespace {
// The line's frequency.
constexpr uint32_t TICK_CYCLES = F_CPU / 60;
}  // namespace

const char *Kw11l::name() const {
    return "KW11L";
}

const char *Kw11l::description() const {
    return "KW11-L";
}

void Kw11l::print() const {
    cli.print("  LKS=");
    cli.printNum(peek(_base_addr), 8, 6);
    cli.println(_request ? " requesting" : "");
}

void Kw11l::reset() {
    _monitor = _ie = false;
    acknowledge();
}

// A tick is not made up for: the line went on while the CPU was halted.
bool Kw11l::loop() {
    if (!_enabled)
        return false;
    const auto now = ARM_DWT_CYCCNT;
    if (now - _tick < TICK_CYCLES)
        return true;
    _tick = (now - _tick < 2 * TICK_CYCLES) ? _tick + TICK_CYCLES : now;
    _monitor = true;
    if (_ie && !_request) {
        _request = true;
        Debugger.target().assertInt(VECTOR);
    }
    return true;
}

void Kw11l::enable(bool enabled) {
    if (!enabled)
        reset();
    Device::enable(enabled);
}

void Kw11l::acknowledge() {
    if (_request) {
        _request = false;
        Debugger.target().negateInt(VECTOR);
    }
}

uint16_t Kw11l::peek(uint32_t) const {
    return (_monitor ? MONITOR_bm : 0) | (_ie ? IE_bm : 0);
}

// MONITOR can only be cleared. The high byte holds nothing.
void Kw11l::write(uint32_t addr, uint16_t data) {
    if (addr != _base_addr)
        return;
    if ((data & MONITOR_bm) == 0)
        _monitor = false;
    _ie = (data & IE_bm) != 0;
    if (!_ie)
        acknowledge();
}

}  // namespace pdp11
}  // namespace debugger

// Local Variables:
// mode: c++
// c-basic-offset: 4
// tab-width: 4
// End:
// vim: set ft=cpp et ts=4 sw=4:
