#include "regs_pdp11.h"

#include <dec_float.h>
#include <str_buffer.h>
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
const char line1f[] = "PC=xxxxxx SP=xxxxxx PS=xxx TNZVC FPS=xxxxxx";
const char line2[] = "R0=xxxxxx R1=xxxxxx R2=xxxxxx R3=xxxxxx R4=xxxxxx R5=xxxxxx";
// The left column of the accumulators, so the right one lines up.
constexpr auto ac_width = 26;
// clang-format on

// Where the save sequence stores; captured, never in memory. The CPU
// reads it before each write.
constexpr uint16_t STORE = 0177000;
}  // namespace

RegsPdp11::RegsPdp11(PinsPdp11 *pins, bool fpu)
    : _pins(pins),
      _fpu(fpu),
      _buffer1(fpu ? line1f : line1),
      _buffer2(line2) {}

void RegsPdp11::print() const {
    auto status = _buffer1;
    status.oct18(3, _pc);
    status.oct18(13, _sp);
    status.oct3(23, _ps >> 6);
    status.oct6(24, _ps);
    status.bits(27, _ps, InstPdp11::PSW_T, "TNZVC");
    if (_fpu)
        status.oct18(37, _fps);
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
    if (_fpu)
        saveFpu();
}

// AC4 and AC5 are reached only through AC0, which is stored first. FPS
// goes back on the restore.
void RegsPdp11::saveFpu() {
    uint16_t seq[2 + 1 + 2 * 4 + 3 * 2 + 2];
    auto n = 0u;
    seq[n++] = InstPdp11::STFPS_ABS;
    seq[n++] = STORE;
    seq[n++] = InstPdp11::SETD;
    for (uint8_t ac = 0; ac < 4; ++ac) {
        seq[n++] = InstPdp11::stdAbs(ac);
        seq[n++] = STORE;
    }
    for (auto i = 0; i < 2; ++i) {
        seq[n++] = InstPdp11::LDD_AC4 + i;
        seq[n++] = InstPdp11::stdAbs(0);
        seq[n++] = STORE;
    }
    seq[n++] = InstPdp11::JMP_ABS;
    seq[n++] = _parkedAt;
    uint16_t buf[1 + 6 * 4];
    auto org = _parkedAt;
    _pins->captureWrites(seq, n, buf, sizeof(buf) / sizeof(buf[0]), org);
    _fps = buf[0];
    for (auto ac = 0; ac < 6; ++ac) {
        for (auto i = 0; i < 4; ++i)
            _ac[ac][i] = buf[1 + 4 * ac + i];
    }
    _parkedAt = org;
}

// The debugger answers the loads from STORE: AC4 and AC5 through AC0
// first, then AC0-AC3 and FPS.
void RegsPdp11::restoreFpu() {
    uint16_t data[6 * 4 + 1];
    uint16_t seq[1 + 2 * 3 + 2 * 4 + 2 + 2];
    auto n = 0u;
    seq[n++] = InstPdp11::SETD;
    for (auto i = 0; i < 2; ++i) {
        seq[n++] = InstPdp11::lddAbs(0);
        seq[n++] = STORE + 8 * i;
        seq[n++] = InstPdp11::STD_AC4 + i;
    }
    for (uint8_t ac = 0; ac < 4; ++ac) {
        seq[n++] = InstPdp11::lddAbs(ac);
        seq[n++] = STORE + 8 * (2 + ac);
    }
    seq[n++] = InstPdp11::LDFPS_ABS;
    seq[n++] = STORE + 8 * 6;
    seq[n++] = InstPdp11::JMP_ABS;
    seq[n++] = _parkedAt;
    static constexpr uint8_t ORDER[] = {4, 5, 0, 1, 2, 3};
    for (auto k = 0; k < 6; ++k) {
        for (auto i = 0; i < 4; ++i)
            data[4 * k + i] = _ac[ORDER[k]][i];
    }
    data[6 * 4] = _fps;
    auto org = _parkedAt;
    _pins->execInst(seq, n, org, org, data, STORE, sizeof(data) / sizeof(data[0]));
    _parkedAt = org;
}

void RegsPdp11::restore() {
    if (_fpu)
        restoreFpu();
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

void RegsPdp11::printAll() const {
    print();
    if (!_fpu)
        return;
    for (auto ac = 0; ac < 6; ++ac) {
        const uint64_t bits = (uint64_t(_ac[ac][0]) << 48) |
                              (uint64_t(_ac[ac][1]) << 32) |
                              (uint32_t(_ac[ac][2]) << 16) | _ac[ac][3];
        libasm::dec_float64_t d;
        d.set(bits);
        char buf[40];
        libasm::StrBuffer str(buf, sizeof(buf));
        str.float64(libasm::float80_t(d));
        cli.print("AC");
        cli.print(char('0' + ac));
        cli.print('=');
        cli.print(buf);
        for (auto w = str.len(); ac % 2 == 0 && w < ac_width; ++w)
            cli.print(' ');
        if (ac % 2) {
            cli.println();
            _pins->idle();
        }
    }
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
