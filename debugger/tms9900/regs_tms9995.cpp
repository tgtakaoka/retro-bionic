#include "regs_tms9995.h"
#include "debugger.h"
#include "inst_tms9900.h"
#include "pins_tms9995.h"

namespace debugger {
namespace tms9995 {

RegsTms9995::RegsTms9995(PinsTms9995 *pins, Mems *mems)
    : RegsTms9900(pins, mems) {}

const char *RegsTms9995::cpu() const {
    return "TMS9995";
}

void RegsTms9995::reset() {
    RegsTms9900::reset();
    auto p = pins<PinsTms9995>();
    p->internal_write16(0xFFFC, 0x1234);  // NMI WP
    p->internal_write16(0xFFFE, 0x5678);  // NMI PC
}

void RegsTms9995::restore() {
    const uint16_t RTWP[] = {
            0x0380,  // RTWP
            _pc,     // WR14: new PC
            _st,     // WR15: new ST
            _wp,     // WR13: new WP
    };
    _pins->injectReads(RTWP, length(RTWP));
}

void RegsTms9995::save() {
    uint16_t buf[3];
    _pins->injectReads(&ZERO, 1);            // inject new WP
    _pins->injectReads(&ZERO, 1);            // inject new PC
    _pins->captureWrites(buf, length(buf));  // capture old context
    _wp = buf[0];                            // WR13: old WP
    _pc = buf[1];                            // WR14: old PC
    _st = buf[2];                            // WR15: old ST
}

void RegsTms9995::breakPoint() {
    uint16_t buf[3];
    _pins->injectReads(&ZERO, 1);            // inject new WP
    _pins->captureWrites(buf, 1);            // WR11: capture new R11
    _pins->injectReads(&ZERO, 1);            // inject new PC
    _pins->captureWrites(buf, length(buf));  // capture old context
    _wp = buf[0];                            // WR13: old WP
    _pc = buf[1];                            // WR14: old PC
    _st = buf[2];                            // WR15: old ST
    _pc -= 2;                                // offset break point XOP
}
uint16_t RegsTms9995::read_reg(uint8_t i) const {
    const auto addr = _wp + i * 2;
    auto p = pins<PinsTms9995>();
    if (p->is_internal(addr))
        return p->internal_read16(addr);
    return RegsTms9900::read_reg(i);
}

void RegsTms9995::write_reg(uint8_t i, uint16_t data) const {
    const auto addr = _wp + i * 2;
    auto p = pins<PinsTms9995>();
    if (p->is_internal(addr)) {
        p->internal_write16(addr, data);
    } else {
        RegsTms9900::write_reg(i, data);
    }
}

}  // namespace tms9995
}  // namespace debugger

// Local Variables:
// mode: c++
// c-basic-offset: 4
// tab-width: 4
// End:
// vim: set ft=cpp et ts=4 sw=4:
