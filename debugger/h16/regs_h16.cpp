#include "regs_h16.h"

#include "debugger.h"
#include "inst_h16.h"
#include "mems_h16.h"
#include "pins_h16.h"

namespace debugger {
namespace h16 {
namespace {
// clang-format off
//                              1         2         3         4         5         6         7         8
//                    012345678901234567890123456789012345678901234567890123456789012345678901234567890
const char line1[] = " PC=xxxxxxxx  SR=T_SF I=x CCR=XNZVC    EBR=xxxxxxxx RBR=xxxxxxxx IBR=xxxxxxxx BMR=xx";
const char line2[] = " R0=xxxxxxxx  R1=xxxxxxxx  R2=xxxxxxxx  R3=xxxxxxxx  R4=xxxxxxxx  R5=xxxxxxxx GBNR=x";
const char line3[] = " R6=xxxxxxxx  R7=xxxxxxxx  R8=xxxxxxxx  R9=xxxxxxxx R10=xxxxxxxx R11=xxxxxxxx VBNR=x";
const char line4[] = "R12=xxxxxxxx R13=xxxxxxxx R14=xxxxxxxx  SP=xxxxxxxx USP=xxxxxxxx BSP=xxxxxxxx CBNR=x";
// clang-format on
// Every register's value starts 13 columns after the one above it, and
// the bank registers' at the end of a line.
constexpr uint8_t POS[] = {4, 17, 30, 43, 56, 69};
constexpr uint8_t BANK = 83;

constexpr uint32_t ADDR_MASK = MemsH16::ADDR_MASK;
// BMR's internal RAM enable, bank permit, which lets the banks be read as
// RAM, and bank mode, ring banks (User's Manual Figure 15-2).
constexpr uint8_t BMR_RAME = 0x80;
constexpr uint8_t BMR_BPM = 0x40;
constexpr uint8_t BMR_BMD = 0x10;
// Where an internal read is stored to, off chip and captured.
constexpr uint32_t SCRATCH = 0xFFD000;

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
      _buffer4(line4) {}

// R15 shows as SP, the stack pointer in use, the other one beside it.
// GBNR, VBNR and CBNR show the bank number bits only.
void RegsH16::print() const {
    auto b1 = _buffer1;
    b1.hex32(POS[0], _pc);
    b1.bits(17, _sr >> 12, 0x8, line1 + 17);
    b1.hex4(24, (_sr >> 8) & 7);
    b1.bits(30, _sr, 0x10, line1 + 30);
    b1.hex32(POS[3], _ebr);
    b1.hex32(POS[4], _rbr);
    b1.hex32(POS[5], _ibr);
    b1.hex8(BANK - 1, _bmr);
    cli.println(b1);
    _pins->idle();
    auto b2 = _buffer2;
    for (uint_fast8_t i = 0; i < 6; ++i)
        b2.hex32(POS[i], _r[i]);
    b2.hex4(BANK, _gbnr & 0xF);
    cli.println(b2);
    _pins->idle();
    auto b3 = _buffer3;
    for (uint_fast8_t i = 0; i < 6; ++i)
        b3.hex32(POS[i], _r[6 + i]);
    b3.hex4(BANK, _vbnr & 7);
    cli.println(b3);
    _pins->idle();
    const auto super = (_sr & InstH16::SR_S) != 0;
    auto b4 = _buffer4;
    for (uint_fast8_t i = 0; i < 3; ++i)
        b4.hex32(POS[i], _r[12 + i]);
    b4.hex32(POS[3], super ? _ssp : _usp);
    b4[POS[4] - 4] = super ? 'U' : 'S';
    b4.hex32(POS[4], super ? _usp : _ssp);
    b4.hex32(POS[5], _bsp);
    b4.hex4(BANK, _cbnr & 7);
    cli.println(b4);
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
    cli.println("?Reg: PC SR CCR SSP USP EBR R0-R14 ICBN DCBN");
}

bool RegsH16::isInternal(uint32_t addr) const {
    addr = wrap(addr);
    if ((_bmr & BMR_RAME) && (addr & ~UINT32_C(0x3FF)) == (_rbr & 0xFFFC00))
        return true;
    return (addr & 0xFF0000) == (_ibr & 0xFF0000) && (addr & 0xFFFF) >= 0xFE00;
}

// A memory-to-memory MOV on the parked CPU, its registers untouched; a
// read stores to |SCRATCH|, off chip, and the write is captured.
uint8_t RegsH16::internalRead(uint32_t addr) {
    // clang-format off
    const uint8_t MOV[] = {
        0x0C, 0x77, 0x00, uint8_t(addr >> 16), uint8_t(addr >> 8),
        uint8_t(addr),                         // MOV:G.B @addr,
        0x77, 0x00, uint8_t(SCRATCH >> 16), uint8_t(SCRATCH >> 8),
        uint8_t(SCRATCH),                      //   @SCRATCH
        0x99, 0xFF, 0xF2,                      // BRA org
    };
    // clang-format on
    static_assert(sizeof(MOV) == 14, "BRA org: minus the length");
    if (!parkAtFetch())
        return 0;
    uint8_t data = 0;
    uint32_t at;
    auto org = _parkedAddr;
    const auto n = _pins->captureWrites(
            MOV, sizeof(MOV), &data, &at, 1, org, PinsH16::EXIT_ORG);
    _parkedAddr = org;
    return n ? data : 0;
}

void RegsH16::internalWrite(uint32_t addr, uint8_t data) {
    // clang-format off
    const uint8_t MOV[] = {
        0x1C, data,                            // MOV:Q.B #data,
        0x77, 0x00, uint8_t(addr >> 16), uint8_t(addr >> 8),
        uint8_t(addr),                         //   @addr
        0x99, 0xFF, 0xF6,                      // BRA org
    };
    // clang-format on
    static_assert(sizeof(MOV) == 10, "BRA org: minus the length");
    if (!parkAtFetch())
        return;
    auto org = _parkedAddr;
    _pins->execInst(MOV, sizeof(MOV), org, PinsH16::EXIT_ORG);
    _parkedAddr = org;
}

// A sequence runs from a parked fetch: answer a vector read first.
bool RegsH16::parkAtFetch() {
    if (!_vector)
        return true;
    if (!_pins->answerVector(_vecAddr, InstH16::ORG_PARK)) {
        cli.println("?internal: vector not answered");
        return false;
    }
    _vector = false;
    _parkedAddr = InstH16::ORG_PARK;
    return true;
}

// Ring bank |bank|'s R0-R15 from the internal RAM, RB7 lowest, with BPM
// set for the reads only: MOV.L each to |SCRATCH|, captured. Four at a
// time keeps a sequence well inside the cycle ring execute() runs in.
bool RegsH16::readRingBank(uint_fast8_t bank, uint32_t regs[16]) const {
    auto self = const_cast<RegsH16 *>(this);
    if (!self->parkAtFetch())
        return false;
    constexpr uint_fast8_t PER = 4;
    const uint32_t base = (_rbr & 0xFFFC00) + (7 - bank) * 64;
    for (uint_fast8_t r = 0; r < 16; r += PER) {
        uint8_t seq[4 + PER * 11 + 4 + 3];
        uint_fast8_t n = 0;
        const auto ldcBmr = [&](uint8_t v) {  // LDC #v, BMR
            seq[n++] = 0xFB;
            seq[n++] = 0x80;
            seq[n++] = 0x71;
            seq[n++] = v;
        };
        ldcBmr(_bmr | BMR_BPM);
        for (uint_fast8_t i = 0; i < PER; ++i) {
            const auto a = base + 4 * (r + i);  // MOV.L @a, @SCRATCH
            seq[n++] = 0x0E;
            seq[n++] = 0x77;
            seq[n++] = 0x00;
            seq[n++] = a >> 16;
            seq[n++] = a >> 8;
            seq[n++] = a;
            seq[n++] = 0x77;
            seq[n++] = 0x00;
            seq[n++] = SCRATCH >> 16;
            seq[n++] = SCRATCH >> 8;
            seq[n++] = SCRATCH;
        }
        ldcBmr(_bmr);
        const uint16_t back = -(n + 3);  // BRA org: from the end of the BRA
        seq[n++] = 0x99;
        seq[n++] = back >> 8;
        seq[n++] = back;
        uint8_t buf[PER * 4];
        uint32_t addrs[PER * 4];
        auto org = self->_parkedAddr;
        const auto got = _pins->captureWrites(
                seq, n, buf, addrs, sizeof(buf), org, PinsH16::EXIT_ORG);
        self->_parkedAddr = org;
        if (got < sizeof(buf) || _pins->cutShort())
            return false;
        for (uint_fast8_t i = 0; i < PER; ++i) {
            const auto b = buf + 4 * i;
            regs[r + i] = static_cast<uint32_t>(b[0]) << 24 | b[1] << 16 |
                          b[2] << 8 | b[3];
        }
    }
    return true;
}

// Ring banks' registers are no stack pointers: CR15 and PR15 are plain,
// though an overflow takes the low 16 bits of the bank's R15 as its
// register list (Programming Manual 4.3.13, 16.3.37).
void RegsH16::printAll() const {
    print();
    if ((_bmr & BMR_BMD) == 0)
        return;
    const uint_fast8_t current = _cbnr & 7;
    const char prefix[] = {'C', 'P'};
    for (uint_fast8_t b = 0; b < 2; ++b) {
        uint32_t regs[16];
        if (!readRingBank((current - b) & 7, regs)) {
            cli.println("?ring bank not read");
            return;
        }
        // Six to a line, each 14 columns, the labels right-aligned.
        for (uint_fast8_t row = 0; row < 16; row += 6) {
            for (uint_fast8_t i = row; i < row + 6 && i < 16; ++i) {
                if (i != row)
                    cli.print(' ');
                if (i < 10)
                    cli.print(' ');
                cli.print(prefix[b]);
                cli.print('R');
                cli.printDec(i);
                cli.print('=');
                cli.printHex(regs[i], 8);
            }
            cli.println();
            _pins->idle();
        }
    }
}

// ICBN or DCBN |count| times on the parked CPU, in ring mode only, with
// R0-R14 and SSP loaded for them: an overflow stacks a bank through BSP,
// with R15 as its register list. The new bank's registers are saved.
void RegsH16::stepBank(uint_fast8_t count, bool increment) {
    if ((_bmr & BMR_BMD) == 0) {
        cli.println(increment ? "?ICBN: not in ring mode"
                              : "?DCBN: not in ring mode");
        return;
    }
    if (_vector && !_pins->answerVector(_vecAddr, InstH16::ORG_PARK)) {
        cli.println("?bank: vector not answered");
        return;
    }
    const auto general = wrap(_ssp - 15 * 4);
    for (uint_fast8_t i = 0; i < 15; ++i) {
        const auto a = general + 4 * i;
        _mems->write_byte(wrap(a), _r[i] >> 24);
        _mems->write_byte(wrap(a + 1), _r[i] >> 16);
        _mems->write_byte(wrap(a + 2), _r[i] >> 8);
        _mems->write_byte(wrap(a + 3), _r[i]);
    }
    uint8_t seq[32];
    uint_fast8_t n = 0;
    const auto movR15 = [&](uint32_t v) {  // MOV.L #v, R15
        seq[n++] = 0x0E;
        seq[n++] = 0x73;
        seq[n++] = v >> 24;
        seq[n++] = v >> 16;
        seq[n++] = v >> 8;
        seq[n++] = v;
        seq[n++] = 0x4F;
    };
    movR15(general);
    seq[n++] = 0x76;  // LDM.L @R15+, R0-R14
    seq[n++] = 0x5F;
    seq[n++] = 0x7F;
    seq[n++] = 0xFF;
    movR15(_ssp);
    for (uint_fast8_t i = 0; i < count; ++i)
        seq[n++] = increment ? InstH16::ICBN : InstH16::DCBN;
    seq[n++] = 0x99;  // BRA org: the displacement is from its end
    seq[n++] = 0xFF;
    seq[n] = uint8_t(-(n + 1));
    ++n;
    auto org = _vector ? InstH16::ORG_PARK : _parkedAddr;
    _pins->execStored(seq, n, org, PinsH16::EXIT_ORG);
    _vector = false;
    _parkedAddr = org;
    if (_pins->cutShort()) {
        cli.println("?bank: sequence cut short");
        return;
    }
    save();
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
constexpr const char *BANKS[] = {
        "ICBN",  // 22
        "DCBN",  // 23
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
            {BANKS, 2, 22, 7},
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
    case 22:
    case 23:
        stepBank(value, reg == 22);
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
