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
    pins<PinsTms9995>()->internal_write16(0xFFFC, 0x1234);  // NMI WP
    pins<PinsTms9995>()->internal_write16(0xFFFE, 0x5678);  // NMI PC
}

void RegsTms9995::restore() {
    const uint8_t RTWP[] = {
            0x03,  // RTWP
            0x80,
            hi(_pc),  // WR14: new PC
            lo(_pc),
            hi(_st),  // WR15: new ST
            lo(_st),
            hi(_wp),  // WR13: new WP
            lo(_wp),
    };
    _pins->injectReads(RTWP, sizeof(RTWP));
}

void RegsTms9995::save() {
    uint8_t buf[sizeof(uint16_t) * 3];
    _pins->injectReads(ZERO, sizeof(ZERO));  // inject new WP
    _pins->injectReads(ZERO, sizeof(ZERO));  // inject new PC
    _pins->captureWrites(buf, sizeof(buf));  // capture old context
    _wp = be16(buf + 0);                     // WR13: old WP
    _pc = be16(buf + 2);                     // WR14: old PC
    _st = be16(buf + 4);                     // WR15: old ST
}

void RegsTms9995::breakPoint() {
    uint8_t buf[sizeof(uint16_t) * 3];
    _pins->injectReads(ZERO, sizeof(ZERO));   // inject new WP
    _pins->captureWrites(buf, sizeof(ZERO));  // WR11: capture new R11
    _pins->injectReads(ZERO, sizeof(ZERO));   // inject new PC
    _pins->captureWrites(buf, sizeof(buf));   // capture old context
    _wp = be16(buf + 0);                      // WR13: old WP
    _pc = be16(buf + 2);                      // WR14: old PC
    _st = be16(buf + 4);                      // WR15: old ST
    _pc -= 2;                                 // offset break point XOP
}

uint16_t RegsTms9995::read_reg(uint8_t i) const {
    const auto addr = _wp + i * 2;
    if (pins<PinsTms9995>()->is_internal(addr))
        return pins<PinsTms9995>()->internal_read16(addr);
    return RegsTms9900::read_reg(i);
}

void RegsTms9995::write_reg(uint8_t i, uint16_t data) const {
    const auto addr = _wp + i * 2;
    if (pins<PinsTms9995>()->is_internal(addr)) {
        pins<PinsTms9995>()->internal_write16(addr, data);
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
