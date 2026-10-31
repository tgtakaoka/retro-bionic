#include "regs_h8500.h"

#include "debugger.h"
#include "inst_h8500.h"
#include "mems_h8500.h"
#include "pins_h8500.h"

namespace debugger {
namespace h8500 {
namespace {
// clang-format off
//                              1         2         3         4         5
//                    012345678901234567890123456789012345678901234567890123
const char line1[] = "PC=xx:xxxx SR=T I=x CCR=NZVC BR=xx EP=xx DP=xx TP=xx";
const char line2[] = "R0=xxxx R1=xxxx R2=xxxx R3=xxxx R4=xxxx R5=xxxx R6=xxxx SP=xxxx";
// clang-format on

// Captured bytes by address: the words an STM stored.
struct Captured {
    const uint8_t *buf;
    const uint32_t *addrs;
    uint_fast8_t from;
    uint_fast8_t to;

    bool readByte(uint32_t addr, uint8_t &v) const {
        for (auto i = from; i < to; ++i) {
            if (addrs[i] == addr) {
                v = buf[i];
                return true;
            }
        }
        return false;
    }
    bool readWord(uint32_t addr, uint16_t &v) const {
        uint8_t h, l;
        if (!readByte(addr, h) || !readByte(addr + 1, l))
            return false;
        v = static_cast<uint16_t>(h) << 8 | l;
        return true;
    }
    // The lowest address stored: R0's, the first register of the list
    // (Programming Manual 2.2.51).
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

RegsH8500::RegsH8500(PinsH8500 *pins, MemsH8500 *mems)
    : _pins(pins),
      _mems(mems),
      _parkedAddr(InstH8500::ORG_PARK),
      _vecAddr(0),
      _pc(InstH8500::ORG_PARK),
      _sr(InstH8500::SR_I),
      _r{0},
      _br(0),
      _ep(0),
      _dp(0),
      _tp(InstH8500::STACK_PARK >> 16),
      _buffer1(line1),
      _buffer2(line2) {
    _r[7] = static_cast<uint16_t>(InstH8500::STACK_PARK);
}

const char *RegsH8500::cpuName() const {
    return _pins->chip().name;
}

void RegsH8500::print() const {
    auto b1 = _buffer1;
    b1.hex8(3, _pc >> 16);
    b1.hex16(6, _pc);
    b1.bits(14, _sr >> 15, 0x1, line1 + 14);
    b1.hex4(18, (_sr >> 8) & 7);
    b1.bits(24, _sr, 0x8, line1 + 24);
    b1.hex8(32, _br);
    b1.hex8(38, _ep);
    b1.hex8(44, _dp);
    b1.hex8(50, _tp);
    cli.println(b1);
    _pins->idle();
    auto b2 = _buffer2;
    for (uint_fast8_t i = 0; i < 8; ++i)
        b2.hex16(3 + 8 * i, _r[i]);
    cli.println(b2);
    _pins->idle();
}

// STM stores R0-R7 below SP, R0 lowest and R7 as SP-2 (Programming Manual
// 2.2.51); then the page registers STC copies into R0-R3 go below those.
// BRA back to the origin fetches it again once both have run. CP has no
// STC: it comes from a frame, or is the park page.
void RegsH8500::save() {
    // clang-format off
    static constexpr uint8_t SAVE[] = {
        0x12, 0xFF,  // STM   (R0-R7), @-SP
        0xA0, 0x9B,  // STC.B BR, R0
        0xA1, 0x9C,  // STC.B EP, R1
        0xA2, 0x9D,  // STC.B DP, R2
        0xA3, 0x9F,  // STC.B TP, R3
        0x12, 0x0F,  // STM   (R0-R3), @-SP
        0x20, 0xF2,  // BRA   org
    };
    // clang-format on
    static_assert(SAVE[sizeof(SAVE) - 1] == uint8_t(-sizeof(SAVE)),
            "BRA org: the displacement is from the end of the BRA");
    constexpr uint_fast8_t GENERAL = 8 * 2;
    constexpr uint_fast8_t PAGE = 4 * 2;
    if (_vector && !_pins->answerVector(_vecAddr, InstH8500::ORG_PARK)) {
        cli.println("?save: vector not answered");
        return;
    }
    auto org = _vector ? InstH8500::ORG_PARK : _parkedAddr;
    uint8_t buf[GENERAL + PAGE];
    uint32_t addrs[sizeof(buf)];
    const auto n = _pins->captureWrites(SAVE, sizeof(SAVE), buf, addrs,
            sizeof(buf), org, PinsH8500::EXIT_ORG);
    if (n < sizeof(buf) || _pins->cutShort()) {
        cli.println("?save: registers not stored");
        return;
    }
    const Captured general{buf, addrs, 0, GENERAL};
    const Captured page{buf, addrs, GENERAL, sizeof(buf)};
    const auto r0 = general.base();
    uint16_t r[8];
    for (uint_fast8_t i = 0; i < 8; ++i)
        general.readWord(r0 + 2 * i, r[i]);
    const auto p0 = page.base();
    uint16_t p[4];
    for (uint_fast8_t i = 0; i < 4; ++i)
        page.readWord(p0 + 2 * i, p[i]);
    for (uint_fast8_t i = 0; i < 7; ++i)
        _r[i] = r[i];
    _br = p[0];
    _ep = p[1];
    _dp = p[2];
    _tp = p[3];
    if (!_vector) {
        // Parked at a fetch outside a vector: SP is where STM began.
        _r[7] = r[7] + 2;
    }
    // Either way the CPU is parked at |org| again, outside a vector.
    _vector = false;
    _parkedAddr = org;
}

void RegsH8500::reset() {
    // The program's reset vector: CP in the low byte of the word at
    // H'0000, then PC. Reset clears T and sets the interrupt mask to 7
    // (Programming Manual 3.4); the stack stays the park one until the
    // program sets its own.
    _pc = static_cast<uint32_t>(_mems->read(InstH8500::VEC_RESET + 1)) << 16 |
          _mems->read16(InstH8500::VEC_RESET + 2);
    _sr = (_sr & 0x00FF) | InstH8500::SR_I;
}

// The SR, CP and PC that RTE takes are staged below SP in TP's page;
// exceptions write there anyway, so it is free stack. The page registers
// and R0-R6 load from immediates, R7 last.
void RegsH8500::restore(bool trace) {
    if (_vector && !_pins->answerVector(_vecAddr, InstH8500::ORG_PARK)) {
        cli.println("?restore: vector not answered");
        return;
    }
    const auto page = static_cast<uint32_t>(_tp) << 16;
    const uint16_t sp = _r[7] - 6;
    const auto stageWord = [&](uint16_t off, uint16_t v) {
        _mems->write_byte(_pins->wrap(page | uint16_t(sp + off)), v >> 8);
        _mems->write_byte(_pins->wrap(page | uint16_t(sp + off + 1)), v);
    };
    stageWord(0, trace ? (_sr | InstH8500::SR_T) : (_sr & ~InstH8500::SR_T));
    stageWord(2, _pc >> 16);  // CP in the low byte
    stageWord(4, _pc);
    // clang-format off
    const uint8_t RESTORE[] = {
        0x04, _br, 0x8B,                          // LDC.B   #br, BR
        0x04, _ep, 0x8C,                          // LDC.B   #ep, EP
        0x04, _dp, 0x8D,                          // LDC.B   #dp, DP
        0x04, _tp, 0x8F,                          // LDC.B   #tp, TP
        0x58, uint8_t(_r[0] >> 8), uint8_t(_r[0]),  // MOV:I.W #r0, R0
        0x59, uint8_t(_r[1] >> 8), uint8_t(_r[1]),
        0x5A, uint8_t(_r[2] >> 8), uint8_t(_r[2]),
        0x5B, uint8_t(_r[3] >> 8), uint8_t(_r[3]),
        0x5C, uint8_t(_r[4] >> 8), uint8_t(_r[4]),
        0x5D, uint8_t(_r[5] >> 8), uint8_t(_r[5]),
        0x5E, uint8_t(_r[6] >> 8), uint8_t(_r[6]),
        0x5F, uint8_t(sp >> 8), uint8_t(sp),      // MOV:I.W #sp-6, R7
        0x0A,                                     // RTE
    };
    // clang-format on
    auto org = _vector ? InstH8500::ORG_PARK : _parkedAddr;
    _pins->execInst(RESTORE, sizeof(RESTORE), org, _pc);
    _vector = false;
    _parkedAddr = org;  // the fetch of the PC, as the bus showed it
}

void RegsH8500::helpRegisters() const {
    cli.println("?Reg: PC SR CCR BR EP DP TP R0-R7 SP");
}

namespace {
constexpr const char *REGS8[] = {
        "CCR",  // 1
        "BR",   // 2
        "EP",   // 3
        "DP",   // 4
        "TP",   // 5
};
constexpr const char *REGS16[] = {
        "SR",  // 6
        "R0",  // 7
        "R1",  // 8
        "R2",  // 9
        "R3",  // 10
        "R4",  // 11
        "R5",  // 12
        "R6",  // 13
        "R7",  // 14
        "SP",  // 15
};
constexpr const char *REGS24[] = {
        "PC",  // 16
};
}  // namespace

const Regs::RegList *RegsH8500::listRegisters(uint_fast8_t n) const {
    static constexpr RegList REG_LIST[] = {
            {REGS8, 5, 1, UINT8_MAX},
            {REGS16, 10, 6, UINT16_MAX},
            {REGS24, 1, 16, 0xFFFFFF},
    };
    return n < sizeof(REG_LIST) / sizeof(REG_LIST[0]) ? &REG_LIST[n] : nullptr;
}

bool RegsH8500::setRegister(uint_fast8_t reg, uint32_t value) {
    switch (reg) {
    case 1:
        _sr = (_sr & 0xFF00) | (value & 0x0F);
        break;
    case 2:
        _br = value;
        break;
    case 3:
        _ep = value;
        break;
    case 4:
        _dp = value;
        break;
    case 5:
        _tp = value;
        break;
    case 6:
        _sr = value;
        break;
    case 15:
        _r[7] = value;
        break;
    case 16:
        _pc = _pins->wrap(value);
        return true;
    default:
        _r[reg - 7] = value;
        break;
    }
    return false;
}

}  // namespace h8500
}  // namespace debugger

// Local Variables:
// mode: c++
// c-basic-offset: 4
// tab-width: 4
// End:
// vim: set ft=cpp et ts=4 sw=4:
