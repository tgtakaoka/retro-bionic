#include "regs_tms9900.h"
#include "debugger.h"
#include "inst_tms9900.h"
#include "pins_tms9900.h"

namespace debugger {
namespace tms9900 {

namespace {
// clang-format off
//                    0123456789012345678901234567890123456789012345678901234567890123456789
const char line1[] = "PC=xxxx  WP=xxxx  ST=LAECVPX_____1111";
const char line2[] = "R0=xxxx  R1=xxxx  R2=xxxx  R3=xxxx  R4=xxxx  R5=xxxx  R6=xxxx  R7=xxxx";
const char line3[] = "R8=xxxx  R9=xxxx R10=xxxx R11=xxxx R12=xxxx R13=xxxx R14=xxxx R15=xxxx";
// clang-format on
}  // namespace

RegsTms9900::RegsTms9900(PinsTms9900 *pins, Mems *mems)
    : _pins(pins),
      _mems(mems),
      _buffer1(line1),
      _buffer2(line2),
      _buffer3(line3) {}

const uint8_t RegsTms9900::ZERO[2] = {0, 0};

const char *RegsTms9900::cpu() const {
    return "TMS9900";
}

const char *RegsTms9900::cpuName() const {
    return Debugger.target().identity();
}

void RegsTms9900::print() const {
    auto reg = _buffer1;
    auto wr1 = _buffer2;
    auto wr2 = _buffer3;
    reg.hex16(3, _pc);
    reg.hex16(12, _wp);
    reg.bits(21, _st, 0x8000, line1 + 21);
    cli.println(reg);
    _pins->idle();
    for (uint8_t i = 0; i < 8; ++i) {
        wr1.hex16(i * 9 + 3, read_reg(i));
        wr2.hex16(i * 9 + 3, read_reg(i + 8));
    }
    cli.println(wr1);
    _pins->idle();
    cli.println(wr2);
    _pins->idle();
}

void RegsTms9900::reset() {
    _wp = _mems->raw_read16(InstTms9900::VEC_RESET + 0);
    _pc = _mems->raw_read16(InstTms9900::VEC_RESET + 2);
}

void RegsTms9900::save() {
    uint8_t buf[sizeof(uint16_t) * 3];
    _pins->injectReads(ZERO, sizeof(ZERO));  // inject new WP
    _pins->captureWrites(buf, sizeof(buf));  // capture old context
    _pins->injectReads(ZERO, sizeof(ZERO));  // inject new PC
    _st = be16(buf + 0);                     // WR15: old ST
    _pc = be16(buf + 2);                     // WR14: old PC
    _wp = be16(buf + 4);                     // WR13: old WP
}

void RegsTms9900::restore() {
    const uint8_t RTWP[] = {
            0x03,  // RTWP
            0x80,
            hi(_st),  // WR15: new ST
            lo(_st),
            hi(_pc),  // WR14: new PC
            lo(_pc),
            hi(_wp),  // WR13: new WP
            lo(_wp),
    };
    _pins->injectReads(RTWP, sizeof(RTWP));
}

void RegsTms9900::breakPoint() {
    _pins->injectReads(ZERO, sizeof(ZERO));  // inject new WP
    uint8_t buf[sizeof(uint16_t) * 4];
    _pins->captureWrites(buf, sizeof(buf));  // capture new R11 and old context
    ;                                        // WR11: new R11
    _st = be16(buf + 2);                     // WR15: old ST
    _pc = be16(buf + 4);                     // WR14: old PC
    _wp = be16(buf + 6);                     // WR13: old WP
    _pins->injectReads(ZERO, sizeof(ZERO));  // inject new PC
    _pc -= 2;                                // offset break point XOP
}

uint16_t RegsTms9900::read_reg(uint8_t i) const {
    return _mems->raw_read16(_wp + i * 2);
}

void RegsTms9900::write_reg(uint8_t i, uint16_t data) const {
    _mems->raw_write16(_wp + i * 2, data);
}

void RegsTms9900::helpRegisters() const {
    cli.println(F("?Reg: PC WP ST, R0~R15"));
}

constexpr const char *REGS16[] = {
        "R0",   // 1
        "R1",   // 2
        "R2",   // 3
        "R3",   // 4
        "R4",   // 5
        "R5",   // 6
        "R6",   // 7
        "R7",   // 8
        "R8",   // 9
        "R9",   // 10
        "R10",  // 11
        "R11",  // 12
        "R12",  // 13
        "R13",  // 14
        "R14",  // 15
        "R15",  // 16
        "PC",   // 17
        "WP",   // 18
        "ST",   // 19
};

const Regs::RegList *RegsTms9900::listRegisters(uint8_t n) const {
    static constexpr RegList REG_LIST[] = {
            {REGS16, 19, 1, UINT16_MAX},
    };
    return n < 1 ? &REG_LIST[n] : nullptr;
}

void RegsTms9900::setRegister(uint8_t reg, uint32_t value) {
    switch (reg) {
    case 17:
        _pc = value;
        break;
    case 18:
        _wp = value;
        break;
    case 19:
        _st = value;
        break;
    default:
        write_reg(reg - 1, value);
        break;
    }
}

}  // namespace tms9900
}  // namespace debugger

// Local Variables:
// mode: c++
// c-basic-offset: 4
// tab-width: 4
// End:
// vim: set ft=cpp et ts=4 sw=4: