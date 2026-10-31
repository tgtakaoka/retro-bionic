#include "regs_h8300.h"

#include "debugger.h"
#include "inst_h8300.h"
#include "mems_h8300.h"
#include "pins_h8300.h"

namespace debugger {
namespace h8300 {
namespace {
// clang-format off
//                              1         2         3         4         5
//                    012345678901234567890123456789012345678901234567890123
const char line1[] = "PC=xxxx SP=xxxx CCR=IUHUNZVC";
const char line2[] = "R0=xxxx R1=xxxx R2=xxxx R3=xxxx R4=xxxx R5=xxxx R6=xxxx";
// clang-format on
}  // namespace

RegsH8300::RegsH8300(PinsH8300 *pins, MemsH8300 *mems)
    : _pins(pins),
      _mems(mems),
      _parkedAddr(InstH8300::ORG_PARK),
      _vecAddr(0),
      _pc(InstH8300::ORG_PARK),
      _ccr(InstH8300::CCR_I),
      _r{0},
      _buffer1(line1),
      _buffer2(line2) {
    _r[7] = InstH8300::STACK_PARK;
}

void RegsH8300::print() const {
    auto &b1 = _buffer1;
    b1.hex16(3, _pc);
    b1.hex16(11, _r[7]);
    b1.bits(20, _ccr, 0x80, line1 + 20);
    cli.println(b1);
    _pins->idle();
    auto &b2 = _buffer2;
    for (uint_fast8_t i = 0; i < 7; ++i)
        b2.hex16(3 + 8 * i, _r[i]);
    cli.println(b2);
    _pins->idle();
}

// Each register goes out as a word to H'0000, high byte first, captured
// and never stored; then CCR through R0L, which is already saved. BRA back
// to the origin fetches it again once all have run.
void RegsH8300::save() {
    // clang-format off
    static constexpr uint8_t SAVE[] = {
        0x6B, 0x80, 0x00, 0x00,  // MOV.W R0, @H'0000:16
        0x6B, 0x81, 0x00, 0x00,  // MOV.W R1, @H'0000:16
        0x6B, 0x82, 0x00, 0x00,  // MOV.W R2, @H'0000:16
        0x6B, 0x83, 0x00, 0x00,  // MOV.W R3, @H'0000:16
        0x6B, 0x84, 0x00, 0x00,  // MOV.W R4, @H'0000:16
        0x6B, 0x85, 0x00, 0x00,  // MOV.W R5, @H'0000:16
        0x6B, 0x86, 0x00, 0x00,  // MOV.W R6, @H'0000:16
        0x6B, 0x87, 0x00, 0x00,  // MOV.W R7, @H'0000:16
        0x02, 0x08,              // STC   CCR, R0L
        0x6A, 0x88, 0x00, 0x00,  // MOV.B R0L, @H'0000:16
        0x40, 0xD8,              // BRA   org
    };
    // clang-format on
    static_assert(SAVE[sizeof(SAVE) - 1] == uint8_t(-sizeof(SAVE)),
            "BRA org: the displacement is from the end of the BRA");
    if (_vector && !_pins->answerVector(_vecAddr, InstH8300::ORG_PARK)) {
        cli.println("?save: vector not answered");
        return;
    }
    auto org = _vector ? InstH8300::ORG_PARK : _parkedAddr;
    uint8_t buf[8 * 2 + 1];
    const auto n = _pins->captureWrites(
            SAVE, sizeof(SAVE), buf, sizeof(buf), org, PinsH8300::EXIT_ORG);
    if (n < sizeof(buf) || _pins->cutShort()) {
        cli.println("?save: registers not stored");
        return;
    }
    for (uint_fast8_t i = 0; i < 7; ++i)
        _r[i] = be16(buf + 2 * i);
    if (!_vector) {
        // Parked at a fetch outside a vector: SP and CCR are the program's.
        _r[7] = be16(buf + 2 * 7);
        _ccr = buf[8 * 2];
    }
    // Either way the CPU is parked at |org| again, outside a vector.
    _vector = false;
    _parkedAddr = org;
}

void RegsH8300::reset() {
    // The program's reset vector. Reset sets the interrupt mask (4.2.2);
    // the stack stays the park one until the program sets its own.
    _pc = _mems->read16(InstH8300::VEC_RESET);
    _ccr |= InstH8300::CCR_I;
}

// R0-R7 load from immediates. LDC holds interrupts off until the JMP has
// fetched the PC's word, so CCR and the PC take effect together (H8/300H
// Software Manual Table 3.1).
void RegsH8300::restore() {
    if (_vector && !_pins->answerVector(_vecAddr, InstH8300::ORG_PARK)) {
        cli.println("?restore: vector not answered");
        return;
    }
    // clang-format off
    const uint8_t RESTORE[] = {
        0x79, 0x00, hi(_r[0]), lo(_r[0]),  // MOV.W #r0, R0
        0x79, 0x01, hi(_r[1]), lo(_r[1]),
        0x79, 0x02, hi(_r[2]), lo(_r[2]),
        0x79, 0x03, hi(_r[3]), lo(_r[3]),
        0x79, 0x04, hi(_r[4]), lo(_r[4]),
        0x79, 0x05, hi(_r[5]), lo(_r[5]),
        0x79, 0x06, hi(_r[6]), lo(_r[6]),
        0x79, 0x07, hi(_r[7]), lo(_r[7]),  // MOV.W #sp, R7
        0x07, _ccr,                        // LDC   #ccr, CCR
        0x5A, 0x00, hi(_pc), lo(_pc),      // JMP   @pc
    };
    // clang-format on
    auto org = _vector ? InstH8300::ORG_PARK : _parkedAddr;
    _pins->execInst(RESTORE, sizeof(RESTORE), org, _pc);
    _vector = false;
    _parkedAddr = org;  // the fetch of the PC, as the bus showed it
}

void RegsH8300::helpRegisters() const {
    cli.println("?Reg: PC SP CCR R0-R7 R0H-R7H R0L-R7L");
}

constexpr const char *REGS8[] = {
        "CCR",  // 1
        "R0H",  // 2
        "R1H",  // 3
        "R2H",  // 4
        "R3H",  // 5
        "R4H",  // 6
        "R5H",  // 7
        "R6H",  // 8
        "R7H",  // 9
        "R0L",  // 10
        "R1L",  // 11
        "R2L",  // 12
        "R3L",  // 13
        "R4L",  // 14
        "R5L",  // 15
        "R6L",  // 16
        "R7L",  // 17
};
constexpr const char *REGS16[] = {
        "PC",  // 18
        "SP",  // 19
        "R0",  // 20
        "R1",  // 21
        "R2",  // 22
        "R3",  // 23
        "R4",  // 24
        "R5",  // 25
        "R6",  // 26
        "R7",  // 27
};

const Regs::RegList *RegsH8300::listRegisters(uint_fast8_t n) const {
    static constexpr RegList REG_LIST[] = {
            {REGS8, 17, 1, UINT8_MAX},
            {REGS16, 10, 18, UINT16_MAX},
    };
    return n < 2 ? &REG_LIST[n] : nullptr;
}

bool RegsH8300::setRegister(uint_fast8_t reg, uint32_t value) {
    if (reg == 1) {
        _ccr = value;
    } else if (reg >= 2 && reg < 10) {
        auto &r = _r[reg - 2];
        r = uint16(value, lo(r));
    } else if (reg >= 10 && reg < 18) {
        auto &r = _r[reg - 10];
        r = uint16(hi(r), value);
    } else if (reg == 18) {
        _pc = value;
        return true;
    } else if (reg == 19) {
        _r[7] = value;
    } else if (reg >= 20) {
        _r[reg - 20] = value;
    }
    return false;
}

}  // namespace h8300
}  // namespace debugger

// Local Variables:
// mode: c++
// c-basic-offset: 4
// tab-width: 4
// End:
// vim: set ft=cpp et ts=4 sw=4:
