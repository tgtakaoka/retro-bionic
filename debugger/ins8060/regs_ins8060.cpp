#include "regs_ins8060.h"
#include "char_buffer.h"
#include "debugger.h"
#include "digital_bus.h"
#include "inst_ins8060.h"
#include "mems_ins8060.h"
#include "pins_ins8060.h"
#include "signals_ins8060.h"

namespace debugger {
namespace ins8060 {

struct RegsIns8060 Regs;

const char *RegsIns8060::cpu() const {
    return "INS8060";
}

const char *RegsIns8060::cpuName() const {
    return cpu();
}

void RegsIns8060::print() const {
    static constexpr char line[] =
            "PC=xxxx P1=xxxx P2=xxxx P3=xxxx E=xx A=xx S=CVBAI210";
    //       0123456789012345678901234567890123456789012345678901
    static auto &buffer = *new CharBuffer(line);
    buffer.hex16(3, _pc());
    buffer.hex16(11, _ptr[1]);
    buffer.hex16(19, _ptr[2]);
    buffer.hex16(27, _ptr[3]);
    buffer.hex8(34, _e);
    buffer.hex8(39, _a);
    buffer.bits(44, _s, 0x80, &line[44]);
    cli.println(buffer);
    Pins.idle();
}

void RegsIns8060::save() {
    // clang-format off
    static const uint8_t ST_ALL[] = {
        0xC8, 0xFE,                         // ST $-1
        0x40, 0xC8, 0xFF,                   // LDE, ST $
        0x06, 0xC8, 0xFF,                   // CSA, ST $
        0x31, 0xC8, 0xFF, 0x35, 0xC8, 0xFF, // XPAL P1, ST $, XPAH P1, ST $
        0x32, 0xC8, 0xFF, 0x36, 0xC8, 0xFF, // XPAL P1, ST $, XPAH P1, ST $
        0x33, 0xC8, 0xFF, 0x37, 0xC8, 0xFF, // XPAL P1, ST $, XPAH P1, ST $
    };
    // clang-format on
    static uint8_t buffer[9];
    Pins.captureWrites(
            ST_ALL, sizeof(ST_ALL), &_ptr[0], buffer, sizeof(buffer));
    _a = buffer[0];
    _e = buffer[1];
    _s = buffer[2];
    _ptr[1] = le16(buffer + 3);
    _ptr[2] = le16(buffer + 5);
    _ptr[3] = le16(buffer + 7);
}

void RegsIns8060::restore() {
    // clang-format off
    static uint8_t LD_ALL[] = {
        0xC4, 0, 0x07,                // LDI s, CAS; s=1
        0xC4, 0, 0x01,                // LDI e, XAE; e=4
        0xC4, 0, 0x33, 0xC4, 0, 0x37, // LDI lo(p3), XPAL P1, LDI hi(p3), XPAH P1
        0xC4, 0, 0x32, 0xC4, 0, 0x36, // LDI lo(p2), XPAL P1, LDI hi(p2), XPAH P1
        0xC4, 0, 0x31, 0xC4, 0, 0x35, // LDI lo(pc), XPAL P1, LDI hi(pc), XPAH P1
        0x3D,                         // XPPC P1
        0xC4, 0, 0x31, 0xC4, 0, 0x35, // LDI lo(p1), XPAL P1, LDI hi(p1), XPAH P1
        0xC4, 0,                      // LDI a
    };
    // clang-format on
    LD_ALL[1] = _s;
    LD_ALL[4] = _e;
    LD_ALL[7] = lo(_ptr[3]);
    LD_ALL[10] = hi(_ptr[3]);
    LD_ALL[13] = lo(_ptr[2]);
    LD_ALL[16] = hi(_ptr[2]);
    const auto pc = _addr(_pc(), _pc() - 8);  // offset restore P1 and A
    LD_ALL[19] = lo(pc);
    LD_ALL[22] = hi(pc);
    LD_ALL[26] = lo(_ptr[1]);
    LD_ALL[29] = hi(_ptr[1]);
    LD_ALL[32] = _a;
    Pins.execInst(LD_ALL, sizeof(LD_ALL));
}

void RegsIns8060::helpRegisters() const {
    cli.println(F("?Reg: PC P1 P2 P3 A E S"));
}

constexpr const char *REGS8[] = {
        "A",  // 1
        "E",  // 2
        "S",  // 3
};
constexpr const char *REGS16[] = {
        "PC",  // 4
        "P1",  // 5
        "P2",  // 6
        "P3",  // 7
};

const Regs::RegList *RegsIns8060::listRegisters(uint8_t n) const {
    static constexpr RegList REG_LIST[] = {
            {REGS8, 3, 1, UINT8_MAX},
            {REGS16, 4, 4, UINT16_MAX},
    };
    return n < 2 ? &REG_LIST[n] : nullptr;
}

void RegsIns8060::setRegister(uint8_t reg, uint32_t value) {
    switch (reg) {
    default:
        _ptr[reg - 4U] = value;
        break;
    case 1:
        _a = value;
        break;
    case 2:
        _e = value;
        break;
    case 3:
        _s = value;
        break;
    }
}

}  // namespace ins8060
}  // namespace debugger

// Local Variables:
// mode: c++
// c-basic-offset: 4
// tab-width: 4
// End:
// vim: set ft=cpp et ts=4 sw=4:
