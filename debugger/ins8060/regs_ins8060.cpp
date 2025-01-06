#include "regs_ins8060.h"
#include "debugger.h"
#include "pins_ins8060.h"
#include "signals_ins8060.h"

namespace debugger {
namespace ins8060 {
namespace {
static constexpr char line[] =
        "PC=xxxx P1=xxxx P2=xxxx P3=xxxx E=xx A=xx S=CVBAI210";
//       0123456789012345678901234567890123456789012345678901
}  // namespace

RegsIns8060::RegsIns8060(PinsIns8060 *pins) : _pins(pins), _buffer(line) {}

const char *RegsIns8060::cpu() const {
    return "INS8060";
}

const char *RegsIns8060::cpuName() const {
    return cpu();
}

void RegsIns8060::print() const {
    _buffer.hex16(3, _pc());
    _buffer.hex16(11, _p1());
    _buffer.hex16(19, _p2());
    _buffer.hex16(27, _p3());
    _buffer.hex8(34, _e);
    _buffer.hex8(39, _a);
    _buffer.bits(44, _s, 0x80, line + 44);
    cli.println(_buffer);
    _pins->idle();
}

void RegsIns8060::save() {
    static constexpr uint8_t ST_ALL[] = {
            0xC8, 0xFE,                          // ST $-1
            0x40, 0xC8, 0xFF,                    // LDE, ST $
            0x06, 0xC8, 0xFF,                    // CSA, ST $
            0x31, 0xC8, 0xFF, 0x35, 0xC8, 0xFF,  // XPAL P1, ST $, XPAH P1, ST $
            0x32, 0xC8, 0xFF, 0x36, 0xC8, 0xFF,  // XPAL P1, ST $, XPAH P1, ST $
            0x33, 0xC8, 0xFF, 0x37, 0xC8, 0xFF,  // XPAL P1, ST $, XPAH P1, ST $
    };
    uint8_t buffer[9];
    _pins->captureWrites(
            ST_ALL, sizeof(ST_ALL), &_pc(), buffer, sizeof(buffer));
    _a = buffer[0];
    _e = buffer[1];
    _s = buffer[2];
    _p1() = le16(buffer + 3);
    _p2() = le16(buffer + 5);
    _p3() = le16(buffer + 7);
}

void RegsIns8060::restorePtr(uint8_t n, uint16_t val) const {
    const uint8_t LD_PTR[] = {
            0xC4, lo(val),    // LDI lo(Pn)
            uint8(0x30 | n),  // XPAL Pn
            0xC4, hi(val),    // LDI hi(Pn)
            uint8(0x34 | n),  // XPAH Pn
    };
    _pins->execInst(LD_PTR, sizeof(LD_PTR));
}

void RegsIns8060::restore() {
    // clang-format off
    const uint8_t LD_SE[] = {
        0xC4, _s, 0x07,                // LDI s, CAS; s=1
        0xC4, _e, 0x01,                // LDI e, XAE; e=4
    };
    _pins->execInst(LD_SE, sizeof(LD_SE));
    restorePtr(3, _p3());
    restorePtr(2, _p2());
    const auto pc = _addr(_pc(), _pc() - 8);  // offset restore P1 and A
    restorePtr(1, pc);
    static constexpr uint8_t XPPC_P1[] = { 0x3D };
    _pins->execInst(XPPC_P1, sizeof(XPPC_P1));
    restorePtr(1, _p1());
    const uint8_t LD_A[] = {
        0xC4, _a,               // LDI _a
    };
    _pins->execInst(LD_A, sizeof(LD_A));
}

void RegsIns8060::helpRegisters() const {
    cli.println("?Reg: PC P1 P2 P3 A E S");
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
