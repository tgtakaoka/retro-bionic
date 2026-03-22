#include "regs_z280.h"
#include "debugger.h"
#include "inst_z280.h"
#include "mems_z280.h"
#include "pins_z280.h"

namespace debugger {
namespace z280 {
namespace {
// clang-format off
//                              1         2         3         4         5         6         7
//                    0123456789012345678901234567890123456789012345678901234567890123456789
const char line1[] = "PC=xxxx SP=xxxx  BC=xxxx DE=xxxx HL=xxxx A=xx F=SZ1H1VNC  I=xx";
const char line2[] = "IX=xxxx IY=xxxx (BC=xxxx DE=xxxx HL=xxxx A=xx F=SZ1H1VNC) R=xx";
const char line3[] = "       USP=xxxx MSR=xxxx IOP=xx";
// clang-format on
}  // namespace

RegsZ280::RegsZ280(PinsZ280 *pins, MemsZ280 *mems)
    : _pins(pins), _mems(mems), _buffer1(line1), _buffer2(line2), _buffer3(line3) {}

void RegsZ280::print() const {
    auto main = _buffer1;
    main.hex16(3, _pc);
    main.hex16(11, _sp);
    main.hex16(20, _main.bc());
    main.hex16(28, _main.de());
    main.hex16(36, _main.hl());
    main.hex8(43, _main.a);
    main.bits(48, _main.f, 0x80, main + 48);
    main.hex8(60, _i);
    cli.println(main);
    _pins->idle();
    auto alt = _buffer2;
    alt.hex16(3, _ix);
    alt.hex16(11, _iy);
    alt.hex16(20, _alt.bc());
    alt.hex16(28, _alt.de());
    alt.hex16(36, _alt.hl());
    alt.hex8(43, _alt.a);
    alt.bits(48, _alt.f, 0x80, alt + 48);
    alt.hex8(60, _r);
    cli.println(alt);
    _pins->idle();
    auto other = _buffer3;
    other.hex16(11, _usp);
    other.hex16(20, _msr);
    other.hex8(29, _iop);
    cli.println(other);
    _pins->idle();
}

void RegsZ280::exchangeRegs(uint32_t &org) const {
    // clang-format off
    static constexpr uint8_t EXCHANGE[] = {
        0x08,  // EX AF, AF'
        0xD9,  // EXX
    };
    // clang-format on
    _pins->execInst(EXCHANGE, sizeof(EXCHANGE), org);
}

void RegsZ280::saveRegs(reg &regs, uint32_t &org) const {
    // PUSH is a single 16-bit write, landing as [low, high]. The
    // prefetch reaches the end of the window before the last write
    // lands, so a captured sequence ends in a jump: taken, it flushes
    // the pipeline and fetches the exit again, this time after the
    // writes, which is the read execute() ends on.
    // clang-format off
    static constexpr uint8_t PUSH_ALL[] = {
        0xF5,        // PUSH AF
        0xC5,        // PUSH BC
        0xD5,        // PUSH DE
        0xE5,        // PUSH HL
        0x18, 0x00,  // JR $+2       ; re-fetch the exit after the writes
    };
    // clang-format on
    uint8_t buffer[8];
    _pins->captureWrites(PUSH_ALL, sizeof(PUSH_ALL), buffer, sizeof(buffer), org);
    regs.f = buffer[0];
    regs.a = buffer[1];
    regs.c = buffer[2];
    regs.b = buffer[3];
    regs.e = buffer[4];
    regs.d = buffer[5];
    regs.l = buffer[6];
    regs.h = buffer[7];
}

void RegsZ280::restoreRegs(const reg &regs, uint32_t &org) const {
    // F has no load instruction of its own, so AF has to come back
    // through a real POP. Injection cannot answer that read: it lands
    // at SP, nowhere near the window |inst| occupies, and only reads
    // inside the window are served from it. So the word goes into
    // memory just below the stack and is popped from there. SP ends
    // up exactly where it started, and restore() sets it for real
    // afterwards regardless.
    const auto scratch = static_cast<uint16_t>(_sp - 2);
    _mems->write_byte(scratch, regs.f);
    _mems->write_byte(scratch + 1, regs.a);
    // clang-format off
    const uint8_t RESTORE[] = {
        0x01, regs.c, regs.b,            // LD BC, regs.bc()
        0x11, regs.e, regs.d,            // LD DE, regs.de()
        0x21, regs.l, regs.h,            // LD HL, regs.hl()
        0x31, lo(scratch), hi(scratch),  // LD SP, scratch
        0xF1,                            // POP AF
        0x18, 0x00,                      // JR $+2: exit after the POP, not at its prefetch
    };
    // clang-format on
    _pins->execInst(RESTORE, sizeof(RESTORE), org);
}

void RegsZ280::save() {
    // CALL pushes the current PC as one word -- the resume point --
    // and jumps. RST 0 would be a byte shorter but jumps straight back
    // to the origin, where it is served the RST again and pushes for
    // as long as the guard allows; calling an address clear of the
    // window cannot wander back into it. The pushed value is the
    // origin plus the length of the CALL.
    // clang-format off
    static constexpr uint8_t PUSH_PC[] = {
        0xCD, 0x00, 0x80,  // CALL 8000H
    };
    // clang-format on
    uint8_t buffer[12];
    // Every sequence runs where the previous one left the CPU parked,
    // starting from the address the last exit captured off the bus.
    const auto parked = parkedAt();
    auto org = parked;
    const auto addr = _pins->captureWrites(
            PUSH_PC, sizeof(PUSH_PC), buffer, 2, org, physical(0x8000));
    if (_inNmi) {
        // The program's PC and MSR came with the NMI push; its system
        // SP is above the two words the push took.
        _sp = _sspAddr + 4;
    } else {
        _sp = addr + 2;
        // The pushed PC is logical; the fetch it came from was |parked|.
        park(le16(buffer) - 3, parked);  // offset the injected CALL
    }
    saveRegs(_main, org);
    exchangeRegs(org);
    saveRegs(_alt, org);
    exchangeRegs(org);
    // LDCTL and LD A,I/R have no bus transfer of their own, so the
    // value is read back through a captured LD (HL),A.
    // clang-format off
    static constexpr uint8_t SAVE_OTHERS[] = {
        0xDD, 0xE5,  // PUSH IX
        0xFD, 0xE5,  // PUSH IY
        0xED, 0x87,  // LDCTL HL, USP
        0xE5,        // PUSH HL
        0x0E, 0x00,  // LD C, 00H     ; Master Status register
        0xED, 0x66,  // LDCTL HL, (C)
        0xE5,        // PUSH HL
        0x0E, 0x08,  // LD C, 08H     ; I/O Page register
        0xED, 0x66,  // LDCTL HL, (C)
        0xE5,        // PUSH HL       ; L only, H is undefined
        0xED, 0x57,  // LD A, I
        0x77,        // LD (HL), A   ; captured, reads back I
        0xED, 0x5F,  // LD A, R
        0x77,        // LD (HL), A   ; captured, reads back R
        0x18, 0x00,  // JR $+2       ; re-fetch the exit after the writes
    };
    // clang-format on
    _pins->captureWrites(
            SAVE_OTHERS, sizeof(SAVE_OTHERS), buffer, sizeof(buffer), org);
    _ix = le16(buffer + 0);
    _iy = le16(buffer + 2);
    _usp = le16(buffer + 4);
    // Inside the NMI service the MSR read is the handler's; the
    // program's came with the push.
    if (!_inNmi)
        _msr = le16(buffer + 6);
    _iop = buffer[8];
    _i = buffer[10];
    _r = buffer[11];
    // Leave the CPU parked back where it was, so that between
    // operations "parked at parkedAt()" holds everywhere -- the
    // invariant the next restore(), step or run resumes from.
    const uint16_t back = _inNmi ? (_handler & 0xFFFF) : _pc;
    // clang-format off
    const uint8_t JP_BACK[] = {
        0xC3, lo(back), hi(back),  // JP back
    };
    // clang-format on
    _pins->execInst(JP_BACK, sizeof(JP_BACK), org, parked);
}

void RegsZ280::restore() {
    // Every register here has a load instruction, so none of them
    // needs a faked POP. Following a POP with its payload in the
    // instruction stream, as the Z80 does, relies on injection being a
    // running cursor that every read advances; this target answers by
    // address instead, and a stack read falls outside the window
    // entirely.
    // clang-format off
    const uint8_t LD_OTHERS[] = {
        0x0E, 0x08,                    // LD C, 08H
        0x21, _iop, 0x00,              // LD HL, _iop
        0xED, 0x6E,                    // LDCTL (C), HL  ; I/O Page
        0x3E, _i,                      // LD A, _i
        0xED, 0x47,                    // LD I, A
        0x3E, _r,                      // LD A, _r
        0xED, 0x4F,                    // LD R, A
        0x21, lo(_usp), hi(_usp),      // LD HL, _usp
        0xED, 0x8F,                    // LDCTL USP, HL
        0xFD, 0x21, lo(_iy), hi(_iy),  // LD IY, _iy
        0xDD, 0x21, lo(_ix), hi(_ix),  // LD IX, _ix
    };
    // clang-format on
    auto org = parkedAt();
    _pins->execInst(LD_OTHERS, sizeof(LD_OTHERS), org);
    exchangeRegs(org);
    restoreRegs(_alt, org);
    exchangeRegs(org);
    restoreRegs(_main, org);
    if (_inNmi) {
        // Return the way the NMI came: RETIL pops the MSR and PC the
        // push saved, so write them back first -- either may have been
        // changed -- and point SP at them. The mode and the stack the
        // program was on come back with the MSR, so this works from
        // user mode as well as system mode.
        _mems->write_byte(_sspAddr + 0, lo(_msr));
        _mems->write_byte(_sspAddr + 1, hi(_msr));
        _mems->write_byte(_sspAddr + 2, lo(_pc));
        _mems->write_byte(_sspAddr + 3, hi(_pc));
        const uint16_t ssp = _sspAddr;
        // clang-format off
        const uint8_t RETURN[] = {
            0x31, lo(ssp), hi(ssp),               // LD SP, ssp
            InstZ280::RETN_PREFIX, InstZ280::RETIL,  // RETIL
        };
        // clang-format on
        _inNmi = false;
        _pins->execInst(RETURN, sizeof(RETURN), org, physicalPc());
        return;
    }
    // The MSR last, since it takes effect at once, then BC and HL again,
    // which it needed. SP last of all: the POP in restoreRegs leaves it
    // where it found it, but _sp is what the program is owed.
    // clang-format off
    const uint8_t LD_ALL[] = {
        0x0E, 0x00,                // LD C, 00H
        0x21, lo(_msr), hi(_msr),  // LD HL, _msr
        0xED, 0x6E,                // LDCTL (C), HL  ; MSR
        0x01, _main.c, _main.b,    // LD BC, bc
        0x21, _main.l, _main.h,    // LD HL, hl
        0x31, lo(_sp), hi(_sp),    // LD SP, _sp
        0xC3, lo(_pc), hi(_pc),    // JP _pc
    };
    // clang-format on
    // The JP is where this one leaves off, and parking the CPU in the
    // fetch at _pc is what hands suspend() the opcode it steps over.
    _pins->execInst(LD_ALL, sizeof(LD_ALL), org, physicalPc());
}

void RegsZ280::helpRegisters() const {
    cli.println("?Reg: PC SP USP IX IY BC DE HL A F B C D E H L I R IOP MSR EX EXX");
}

constexpr const char *REGS8[] = {
        "F",  // 1
        "A",  // 2
        "B",  // 3
        "C",  // 4
        "D",  // 5
        "E",  // 6
        "H",  // 7
        "L",  // 8
        "I",  // 9
        "R",  // 10
        "IOP",  // 11
};
constexpr const char *REGS16[] = {
        "PC",   // 12
        "SP",   // 13
        "USP",  // 14
        "BC",   // 15
        "DE",   // 16
        "HL",   // 17
        "IX",   // 18
        "IY",   // 19
        "MSR",  // 20
};
constexpr const char *EXCHANGE[] = {
        "EX",   // 21
        "EXX",  // 22
};

const Regs::RegList *RegsZ280::listRegisters(uint_fast8_t n) const {
    static constexpr RegList REG_LIST[] = {
            {REGS8, 11, 1, UINT8_MAX},
            {REGS16, 9, 12, UINT16_MAX},
            {EXCHANGE, 2, 21, 1},
    };
    return n < 3 ? &REG_LIST[n] : nullptr;
}

bool RegsZ280::setRegister(uint_fast8_t reg, uint32_t value) {
    switch (reg) {
    case 12:
        setPc(value);
        return true;
    case 13:
        _sp = value;
        break;
    case 14:
        _usp = value;
        break;
    case 15:
        _main.setbc(value);
        break;
    case 16:
        _main.setde(value);
        break;
    case 17:
        _main.sethl(value);
        break;
    case 18:
        _ix = value;
        break;
    case 19:
        _iy = value;
        break;
    case 20:
        _msr = value;
        break;
    case 1:
        _main.f = value;
        break;
    case 2:
        _main.a = value;
        break;
    case 3:
        _main.b = value;
        break;
    case 4:
        _main.c = value;
        break;
    case 5:
        _main.d = value;
        break;
    case 6:
        _main.e = value;
        break;
    case 7:
        _main.h = value;
        break;
    case 8:
        _main.l = value;
        break;
    case 9:
        _i = value;
        break;
    case 10:
        _r = value;
        break;
    case 11:
        _iop = value;
        break;
    case 21:
        swap8(_main.a, _alt.a);
        swap8(_main.f, _alt.f);
        break;
    case 22:
        swap8(_main.b, _alt.b);
        swap8(_main.c, _alt.c);
        swap8(_main.d, _alt.d);
        swap8(_main.e, _alt.e);
        swap8(_main.h, _alt.h);
        swap8(_main.l, _alt.l);
        break;
    }
    return false;
}

}  // namespace z280
}  // namespace debugger

// Local Variables:
// mode: c++
// c-basic-offset: 4
// tab-width: 4
// End:
// vim: set ft=cpp et ts=4 sw=4:
