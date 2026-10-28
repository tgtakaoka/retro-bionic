#include "regs_h16.h"

#include "debugger.h"
#include "inst_h16.h"
#include "mems_h16.h"
#include "pins_h16.h"

namespace debugger {
namespace h16 {
namespace {
// clang-format off
//                              1         2         3         4         5         6         7
//                    0123456789012345678901234567890123456789012345678901234567890123456789012
const char line1[] = "PC=xxxxxx SSP=xxxxxxxx USP=xxxxxxxx SR=T_SF I=x CCR=XNZVC";
const char line2[] = "R0=xxxxxxxx R1=xxxxxxxx R2=xxxxxxxx R3=xxxxxxxx R4=xxxxxxxx R5=xxxxxxxx";
const char line3[] = "R6=xxxxxxxx R7=xxxxxxxx R8=xxxxxxxx R9=xxxxxxxx R10=xxxxxxxx R11=xxxxxxxx";
const char line4[] = "R12=xxxxxxxx R13=xxxxxxxx R14=xxxxxxxx BMR=xx GBNR=xx VBNR=xx";
const char line5[] = "EBR=xxxxxxxx RBR=xxxxxxxx IBR=xxxxxxxx BSP=xxxxxxxx CBNR=xxxxxxxx";
// clang-format on

constexpr uint32_t ADDR_MASK = MemsH16::ADDR_MASK;

constexpr uint32_t wrap(uint32_t addr) {
    return addr & ADDR_MASK;
}

// Captured bytes by address: the long words an STM stored, whatever the
// SSP's alignment split them into.
struct Captured {
    const uint8_t *buf;
    const uint32_t *addrs;
    uint_fast8_t from;
    uint_fast8_t to;

    bool readByte(uint32_t addr, uint8_t &v) const {
        for (auto i = from; i < to; ++i) {
            if (addrs[i] == wrap(addr)) {
                v = buf[i];
                return true;
            }
        }
        return false;
    }
    bool readLong(uint32_t addr, uint32_t &v) const {
        uint32_t l = 0;
        for (uint_fast8_t i = 0; i < 4; ++i) {
            uint8_t b;
            if (!readByte(addr + i, b))
                return false;
            l = l << 8 | b;
        }
        v = l;
        return true;
    }
    // The lowest address stored: R0's, the first register of the list.
    uint32_t base() const {
        auto low = addrs[from];
        for (auto i = from + 1; i < to; ++i) {
            if (addrs[i] < low)
                low = addrs[i];
        }
        return low;
    }
};
}  // namespace

RegsH16::RegsH16(PinsH16 *pins, MemsH16 *mems)
    : _pins(pins),
      _mems(mems),
      _parkedAddr(InstH16::ORG_PARK),
      _vecAddr(0),
      _pc(InstH16::ORG_PARK),
      _sr(InstH16::SR_S | 0x0700),
      _ssp(0),
      _usp(0),
      _r{0},
      _ebr(0),
      _rbr(0),
      _ibr(0),
      _bsp(0),
      _cbnr(0),
      _bmr(0),
      _gbnr(0),
      _vbnr(0),
      _buffer1(line1),
      _buffer2(line2),
      _buffer3(line3),
      _buffer4(line4),
      _buffer5(line5) {}

void RegsH16::print() const {
    auto b1 = _buffer1;
    b1.hex24(3, _pc);
    b1.hex32(14, _ssp);
    b1.hex32(27, _usp);
    b1.bits(39, _sr >> 12, 0x8, line1 + 39);
    b1.hex4(46, (_sr >> 8) & 7);
    b1.bits(52, _sr, 0x10, line1 + 52);
    cli.println(b1);
    _pins->idle();
    static constexpr uint8_t POS2[] = {3, 15, 27, 39, 51, 63};
    static constexpr uint8_t POS3[] = {3, 15, 27, 39, 52, 65};
    auto b2 = _buffer2;
    for (uint_fast8_t i = 0; i < 6; ++i)
        b2.hex32(POS2[i], _r[i]);
    cli.println(b2);
    _pins->idle();
    auto b3 = _buffer3;
    for (uint_fast8_t i = 0; i < 6; ++i)
        b3.hex32(POS3[i], _r[6 + i]);
    cli.println(b3);
    _pins->idle();
    auto b4 = _buffer4;
    b4.hex32(4, _r[12]);
    b4.hex32(17, _r[13]);
    b4.hex32(30, _r[14]);
    b4.hex8(43, _bmr);
    b4.hex8(51, _gbnr);
    b4.hex8(59, _vbnr);
    cli.println(b4);
    _pins->idle();
    auto b5 = _buffer5;
    b5.hex32(4, _ebr);
    b5.hex32(17, _rbr);
    b5.hex32(30, _ibr);
    b5.hex32(43, _bsp);
    b5.hex32(57, _cbnr);
    cli.println(b5);
    _pins->idle();
}

// In supervisor mode R15 is SSP. STM stores R0-R14 below it, R0 lowest
// (Programming Manual Figure 16-37), then the control registers STC
// copies into R0-R8 go below those. BRA back to the origin fetches it
// again once both have run.
void RegsH16::save() {
    // clang-format off
    static constexpr uint8_t SAVE[] = {
        0x72, 0x6F, 0x7F, 0xFF,  // STM.L R0-R14, @-R15
        0xFC, 0xC2, 0x40,        // STC   USP, R0
        0xFC, 0xC0, 0x41,        // STC   EBR, R1
        0xFC, 0xC1, 0x42,        // STC   RBR, R2
        0xFC, 0xC3, 0x43,        // STC   IBR, R3
        0xFC, 0x40, 0x44,        // STC   CBNR, R4
        0xFC, 0x41, 0x45,        // STC   BSP, R5
        0xFC, 0x80, 0x46,        // STC   BMR, R6
        0xFC, 0x81, 0x47,        // STC   GBNR, R7
        0xFC, 0x01, 0x48,        // STC   VBNR, R8
        0x72, 0x6F, 0x01, 0xFF,  // STM.L R0-R8, @-R15
        0x99, 0xFF, 0xDA,        // BRA   org
    };
    // clang-format on
    static_assert(SAVE[sizeof(SAVE) - 2] == 0xFF &&
                          SAVE[sizeof(SAVE) - 1] == uint8_t(-sizeof(SAVE)),
            "BRA org: the displacement is from the end of the BRA");
    constexpr uint_fast8_t GENERAL = 15 * 4;
    constexpr uint_fast8_t CONTROL = 9 * 4;
    if (_vector && !_pins->answerVector(_vecAddr, InstH16::ORG_PARK)) {
        cli.println("?save: vector not answered");
        return;
    }
    auto org = _vector ? InstH16::ORG_PARK : _parkedAddr;
    uint8_t buf[GENERAL + CONTROL];
    uint32_t addrs[sizeof(buf)];
    const auto n = _pins->captureWrites(SAVE, sizeof(SAVE), buf, addrs,
            sizeof(buf), org, PinsH16::EXIT_ORG);
    if (n < sizeof(buf) || _pins->cutShort()) {
        cli.println("?save: registers not stored");
        return;
    }
    const Captured general{buf, addrs, 0, GENERAL};
    const Captured control{buf, addrs, GENERAL, sizeof(buf)};
    const auto r0 = general.base();
    for (uint_fast8_t i = 0; i < 15; ++i)
        general.readLong(r0 + 4 * i, _r[i]);
    const auto c0 = control.base();
    uint32_t c[9];
    for (uint_fast8_t i = 0; i < 9; ++i)
        control.readLong(c0 + 4 * i, c[i]);
    _usp = c[0];
    _ebr = c[1];
    _rbr = c[2];
    _ibr = c[3];
    _cbnr = c[4];
    _bsp = c[5];
    _bmr = c[6];
    _gbnr = c[7];
    _vbnr = c[8];
    if (!_vector) {
        // Parked at a fetch outside a vector: SSP is where STM began.
        _ssp = wrap(r0 + GENERAL);
    }
    // Either way the CPU is parked at |org| again, outside a vector.
    _vector = false;
    _parkedAddr = org;
}

void RegsH16::reset() {
    // The program's reset vector: SSP, then PC. Reset sets S and the
    // interrupt mask, and clears T and IF (User's Manual Table 4-12).
    _ssp = static_cast<uint32_t>(_mems->read16(InstH16::VEC_RESET)) << 16 |
           _mems->read16(InstH16::VEC_RESET + 2);
    _pc = wrap(static_cast<uint32_t>(_mems->read16(InstH16::VEC_RESET + 4))
                       << 16 |
               _mems->read16(InstH16::VEC_RESET + 6));
    _sr = (_sr & 0x00FF) | InstH16::SR_S | 0x0700;
}

// Everything comes back from a block staged just below SSP: LDC and LDM
// read it as real memory, and RTE the SR and PC at the top. Exceptions
// write there anyway, so it is free stack.
void RegsH16::restore(bool trace) {
    if (_vector && !_pins->answerVector(_vecAddr, InstH16::ORG_PARK)) {
        cli.println("?restore: vector not answered");
        return;
    }
    const auto stageWord = [this](uint32_t addr, uint16_t v) {
        _mems->write_byte(wrap(addr), v >> 8);
        _mems->write_byte(wrap(addr + 1), v & 0xFF);
    };
    const auto stageLong = [&](uint32_t addr, uint32_t v) {
        stageWord(addr, v >> 16);
        stageWord(addr + 2, v);
    };
    // USP, EBR, R0-R14, then the SR and PC RTE takes (Figure 4-62).
    const auto frame = wrap(_ssp - 6);
    const auto general = wrap(frame - 15 * 4);
    const auto base = wrap(general - 2 * 4);
    stageLong(base, _usp);
    stageLong(base + 4, _ebr);
    for (uint_fast8_t i = 0; i < 15; ++i)
        stageLong(general + 4 * i, _r[i]);
    // A trace stacks the SR with T still set (User's Manual 4.6.1).
    stageWord(frame, trace ? (_sr | InstH16::SR_T) : (_sr & ~InstH16::SR_T));
    stageLong(frame + 2, _pc);
    // clang-format off
    const uint8_t RESTORE[] = {
        0x0E, 0x73, uint8_t(base >> 24), uint8_t(base >> 16),
        uint8_t(base >> 8), uint8_t(base), 0x4F,  // MOV.L #base, R15
        0xFB, 0xC2, 0x5F,                         // LDC   @R15+, USP
        0xFB, 0xC0, 0x5F,                         // LDC   @R15+, EBR
        0x76, 0x5F, 0x7F, 0xFF,                   // LDM.L @R15+, R0-R14
        0xF1,                                     // RTE
    };
    // clang-format on
    auto org = _vector ? InstH16::ORG_PARK : _parkedAddr;
    _pins->execInst(RESTORE, sizeof(RESTORE), org, _pc);
    _vector = false;
    _parkedAddr = org;  // the fetch of the PC, as the bus showed it
}

void RegsH16::helpRegisters() const {
    cli.println("?Reg: PC SR CCR SSP USP EBR R0-R14");
}

namespace {
constexpr const char *REGS8[] = {
        "CCR",  // 1
};
constexpr const char *REGS16[] = {
        "SR",  // 2
};
constexpr const char *REGS24[] = {
        "PC",  // 3
};
constexpr const char *REGS32[] = {
        "SSP",  // 4
        "USP",  // 5
        "EBR",  // 6
        "R0",   // 7
        "R1",   // 8
        "R2",   // 9
        "R3",   // 10
        "R4",   // 11
        "R5",   // 12
        "R6",   // 13
        "R7",   // 14
        "R8",   // 15
        "R9",   // 16
        "R10",  // 17
        "R11",  // 18
        "R12",  // 19
        "R13",  // 20
        "R14",  // 21
};
}  // namespace

const Regs::RegList *RegsH16::listRegisters(uint_fast8_t n) const {
    static constexpr RegList REG_LIST[] = {
            {REGS8, 1, 1, UINT8_MAX},
            {REGS16, 1, 2, UINT16_MAX},
            {REGS24, 1, 3, ADDR_MASK},
            {REGS32, 18, 4, UINT32_MAX},
    };
    return n < sizeof(REG_LIST) / sizeof(REG_LIST[0]) ? &REG_LIST[n] : nullptr;
}

bool RegsH16::setRegister(uint_fast8_t reg, uint32_t value) {
    switch (reg) {
    case 1:
        _sr = (_sr & 0xFF00) | (value & 0x1F);
        break;
    case 2:
        _sr = value;
        break;
    case 3:
        _pc = value;
        return true;
    case 4:
        _ssp = value;
        break;
    case 5:
        _usp = value;
        break;
    case 6:
        _ebr = value;
        break;
    default:
        _r[reg - 7] = value;
        break;
    }
    return false;
}

}  // namespace h16
}  // namespace debugger

// Local Variables:
// mode: c++
// c-basic-offset: 4
// tab-width: 4
// End:
// vim: set ft=cpp et ts=4 sw=4:
