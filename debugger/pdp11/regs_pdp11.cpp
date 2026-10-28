#include "regs_pdp11.h"

#include "debugger.h"
#include "inst_pdp11.h"
#include "pins_pdp11.h"

namespace debugger {
namespace pdp11 {
namespace {
// clang-format off
//                              1         2         3         4         5
//                    0123456789012345678901234567890123456789012345678901234567890
const char line1[] = "PC=xxxxxx SP=xxxxxx PS=xxx TNZVC";
const char line2[] = "R0=xxxxxx R1=xxxxxx R2=xxxxxx R3=xxxxxx R4=xxxxxx R5=xxxxxx";
// clang-format on

// Where the save sequence stores; captured, never in memory. The CPU
// reads it before each write.
constexpr uint16_t STORE = 0177000;
}  // namespace

RegsPdp11::RegsPdp11(PinsPdp11 *pins)
    : _pins(pins), _buffer1(line1), _buffer2(line2) {}

void RegsPdp11::print() const {
    auto status = _buffer1;
    status.oct18(3, _pc);
    status.oct18(13, _sp);
    status.oct3(23, _ps >> 6);
    status.oct6(24, _ps);
    status.bits(27, _ps, InstPdp11::PSW_T, "TNZVC");
    cli.println(status);
    _pins->idle();
    auto regs = _buffer2;
    for (auto i = 0; i < 6; ++i)
        regs.oct18(3 + 10 * i, _r[i]);
    cli.println(regs);
    _pins->idle();
}

void RegsPdp11::save() {
    // A trap's frame holds the PS; otherwise MFPS reads it.
    uint16_t seq[2 * 9];
    auto n = 0u;
    for (uint8_t i = 0; i <= InstPdp11::SP; ++i) {
        seq[n++] = InstPdp11::movAbs(i);
        seq[n++] = STORE;
    }
    if (!_frame) {
        seq[n++] = InstPdp11::MFPS_ABS;
        seq[n++] = STORE;
    }
    seq[n++] = InstPdp11::JMP_ABS;
    seq[n++] = _parkedAt;
    uint16_t buf[8];
    const uint_fast8_t max = _frame ? 7 : 8;
    auto org = _parkedAt;
    _pins->captureWrites(seq, n, buf, max, org);
    for (auto i = 0; i < 6; ++i)
        _r[i] = buf[i];
    _sp = buf[6];
    if (_frame) {
        // The program's SP is above the frame the debugger's trap pushed.
        _sp += 4;
        _frame = false;
    } else {
        _ps = buf[7] & 0377;  // MFPS writes a byte
    }
    _parkedAt = org;
}

void RegsPdp11::restore() {
    // RTT pops the PC and PS from below the program's SP, which the
    // debugger answers; it sets the program's priority and T bit together
    // with the jump, and a T bit traps after one instruction.
    const uint16_t sp = _sp - 4;
    uint16_t seq[2 * 7 + 1];
    auto n = 0u;
    for (uint8_t i = 0; i < 6; ++i) {
        seq[n++] = InstPdp11::movImm(i);
        seq[n++] = _r[i];
    }
    seq[n++] = InstPdp11::movImm(InstPdp11::SP);
    seq[n++] = sp;
    seq[n++] = InstPdp11::RTT;
    const uint16_t ps = _trace ? (_ps | InstPdp11::PSW_T) : _ps;
    const uint16_t frame[] = {_pc, ps};
    auto org = _parkedAt;
    _pins->execInst(seq, n, org, _pc, frame, sp, 2);
    // A pending interrupt the new priority lets in parks at its handler.
    park(org, org);
}

void RegsPdp11::helpRegisters() const {
    cli.println("?Reg: R0-R7 SP PC PS");
}

constexpr const char *REGS16[] = {
        "R0",
        "R1",
        "R2",
        "R3",
        "R4",
        "R5",
        "SP",
        "PC",  // 1-8
        "R6",
        "R7",  // 9-10
        "PS",  // 11
};

const Regs::RegList *RegsPdp11::listRegisters(uint_fast8_t n) const {
    static constexpr RegList REG_LIST[] = {
            {REGS16, 11, 1, UINT16_MAX},
    };
    return n < 1 ? &REG_LIST[n] : nullptr;
}

bool RegsPdp11::setRegister(uint_fast8_t reg, uint32_t value) {
    if (reg >= 1 && reg <= 6) {
        _r[reg - 1] = value;
    } else if (reg == 7 || reg == 9) {
        _sp = value;
    } else if (reg == 8 || reg == 10) {
        _pc = value;
        return true;
    } else if (reg == 11) {
        _ps = value;
    }
    return false;
}

}  // namespace pdp11
}  // namespace debugger

// Local Variables:
// mode: c++
// c-basic-offset: 4
// tab-width: 4
// End:
// vim: set ft=cpp et ts=4 sw=4:
