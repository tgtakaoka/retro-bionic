#include "regs_ns32016.h"

#include <initializer_list>

#include "debugger.h"
#include "inst_ns32016.h"
#include "mems_ns32016.h"
#include "pins_ns32016.h"

namespace debugger {
namespace ns32016 {
namespace {
// clang-format off
//                              1         2         3         4         5         6         7
//                    01234567890123456789012345678901234567890123456789012345678901234567890
const char line1[] = "PC=xxxxxx PSR=xxxx IPSUNZF__LTC MOD=xxxx SB=xxxxxxxx FP=xxxxxxxx";
const char line2[] = "SP0=xxxxxxxx SP1=xxxxxxxx INTBASE=xxxxxxxx";
const char line3[] = "R0=xxxxxxxx R1=xxxxxxxx R2=xxxxxxxx R3=xxxxxxxx";
const char line4[] = "R4=xxxxxxxx R5=xxxxxxxx R6=xxxxxxxx R7=xxxxxxxx";
const char line5[] = "F0=xxxxxxxx F1=xxxxxxxx F2=xxxxxxxx F3=xxxxxxxx";
const char line6[] = "F4=xxxxxxxx F5=xxxxxxxx F6=xxxxxxxx F7=xxxxxxxx FSR=xxxxxxxx";
// clang-format on

// Where the save sequences store, by a one-byte displacement; captured,
// never in memory.
constexpr uint8_t STORE = 0x20;
// Where the restore sequence reads F0-F7, answered by address.
constexpr uint32_t FLOAD = InstNs32016::PARK_PC + 0x800;

// Opcode bytes (Series 32000 Instruction Manual; checked with libasm):
// gen fields are 10101 absolute, 10100 immediate, 00rrr a register, and
// immediates and displacements are big-endian.
constexpr uint8_t ABS = 0x15;
constexpr uint8_t IMM = 0x14;

struct Seq {
    uint8_t buf[160];
    uint_fast8_t len = 0;

    void byte(uint8_t b) { buf[len++] = b; }
    void bytes(std::initializer_list<uint8_t> list) {
        for (auto b : list)
            byte(b);
    }
    void be32(uint32_t v) {
        bytes({uint8_t(v >> 24), uint8_t(v >> 16), uint8_t(v >> 8),
                uint8_t(v)});
    }
    void be16(uint16_t v) { bytes({uint8_t(v >> 8), uint8_t(v)}); }
    // An absolute address as a 4-byte displacement.
    void abs32(uint32_t addr) { be32(0xC0000000 | (addr & 0x3FFFFFFF)); }

    // MOVD Rn, @STORE
    void storeR(uint_fast8_t n) {
        bytes({uint8_t(0x17 | ((ABS & 3) << 6)), uint8_t((n << 3) | (ABS >> 2)),
                STORE});
    }
    // MOVD #imm, Rn
    void loadR(uint_fast8_t n, uint32_t v) {
        bytes({uint8_t(0x17 | ((n & 3) << 6)), uint8_t((IMM << 3) | (n >> 2))});
        be32(v);
    }
    // SPRi areg, @STORE: D 0x2F/0xAF pairs, W 0xAD
    void spr(uint8_t b0, uint8_t b1) { bytes({b0, b1, STORE}); }
    // LPRD areg, #imm
    void lprd(uint8_t b0, uint8_t b1, uint32_t v) {
        bytes({b0, b1});
        be32(v);
    }
    void bisS() { bytes({0x7D, 0xA3, 0x02, 0x00}); }  // BISPSRW 0x200
    void bicS() { bytes({0x7D, 0xA1, 0x02, 0x00}); }  // BICPSRW 0x200
    void bicI() { bytes({0x7D, 0xA1, 0x08, 0x00}); }  // BICPSRW 0x800
    // MOVF Fn, @STORE
    void storeF(uint_fast8_t n) {
        bytes({0xBE, uint8_t(0x05 | ((ABS & 3) << 6)),
                uint8_t((n << 3) | (ABS >> 2)), STORE});
    }
    // MOVF @addr, Fn
    void loadF(uint_fast8_t n, uint32_t addr) {
        bytes({0xBE, uint8_t(0x05 | ((n & 3) << 6)),
                uint8_t((ABS << 3) | (n >> 2))});
        abs32(addr);
    }
    // BR to |to| from the end of the sequence.
    void branch(uint32_t org, uint32_t to) {
        const int32_t d = int32_t(to - (org + len));
        byte(InstNs32016::BR);
        if (d >= -64 && d < 64) {
            byte(d & 0x7F);
        } else if (d >= -8192 && d < 8192) {
            be16(0x8000 | (d & 0x3FFF));
        } else {
            be32(0xC0000000 | (d & 0x3FFFFFFF));
        }
    }
};

uint32_t le32(const uint8_t *p) {
    return p[0] | (p[1] << 8) | (uint32_t(p[2]) << 16) | (uint32_t(p[3]) << 24);
}

}  // namespace

RegsNs32016::RegsNs32016(PinsNs32016 *pins, MemsNs32016 *mems)
    : _pins(pins),
      _mems(mems),
      _buffer1(line1),
      _buffer2(line2),
      _buffer3(line3) {}

const char *RegsNs32016::cpuName() const {
    return _pins->fpu() ? "NS32081" : "NS32016";
}

void RegsNs32016::print() const {
    auto status = _buffer1;
    status.hex24(3, _pc);
    status.hex16(14, _psr);
    status.bits(19, _psr, 0x800, "IPSUNZF");
    status.bits(28, _psr, 0x004, "LTC");
    status.hex16(36, _mod);
    status.hex32(44, _sb);
    status.hex32(56, _fp);
    cli.println(status);
    auto stacks = _buffer2;
    stacks.hex32(4, _sp0);
    stacks.hex32(17, _sp1);
    stacks.hex32(34, _intbase);
    cli.println(stacks);
    for (auto l = 0; l < 2; ++l) {
        auto line = _buffer3;
        line.set(l == 0 ? line3 : line4);
        for (auto i = 0; i < 4; ++i)
            line.hex32(3 + 12 * i, _r[4 * l + i]);
        cli.println(line);
    }
    if (_pins->fpu()) {
        for (auto l = 0; l < 2; ++l) {
            auto line = _buffer3;
            line.set(l == 0 ? line5 : line6);
            for (auto i = 0; i < 4; ++i)
                line.hex32(3 + 12 * i, _f[4 * l + i]);
            if (l == 1)
                line.hex32(52, _fsr);
            cli.println(line);
        }
    }
}

// Parked on the fetch at 0 after reset, in supervisor mode with PSR 0.
// CFG is cleared but for F on the NS32081 board: with F set and no FPU the
// CPU would wait for it forever, and with F clear its instructions trap.
void RegsNs32016::reset() {
    _pc = 0;
    _psr = 0;
    _parkedAt = 0;
    _frame = false;
    _tracing = false;
    Seq seq;
    seq.bytes({0x0E, 0x0B, uint8_t(_pins->fpu() ? 0x01 : 0x00)});  // SETCFG
    seq.branch(_parkedAt, _parkedAt);
    auto org = _parkedAt;
    _pins->execInst(seq.buf, seq.len, org, _parkedAt);
    save();
    // Reset leaves the stack pointers undefined; RETT pops a frame below
    // SP0 by aligned words.
    _sp0 &= ~3;
    _sp1 &= ~3;
}

void RegsNs32016::parkInTrap(uint32_t pc, uint16_t mod, uint16_t psr) {
    _pc = pc;
    _mod = mod;
    // A step's trace trap pushed the debugger's T; the program keeps its own.
    if (_tracing) {
        psr = (psr & ~InstNs32016::PSR_T) | (_psr & InstNs32016::PSR_T);
        _tracing = false;
    }
    _psr = psr & ~InstNs32016::PSR_P;
    _parkedAt = InstNs32016::PARK_PC;
    _frame = true;
}

// SP0 first, as the CPU in the trap (or after reset) runs on it, then SP1
// with S set. PC, PSR and MOD come from a trap's frame; SB is lost to the
// trap's module, and is the program module's, as RETT would load it.
void RegsNs32016::save() {
    Seq seq;
    // BPT and TRC leave I set: no interrupt may break into the sequence.
    seq.bicI();
    for (uint_fast8_t n = 0; n < 8; ++n)
        seq.storeR(n);
    seq.spr(0xAF, 0xAC);  // SPRD SP
    seq.spr(0x2F, 0xAC);  // SPRD FP
    seq.spr(0x2F, 0xAF);  // SPRD INTBASE
    seq.bisS();
    seq.spr(0xAF, 0xAC);  // SPRD SP
    seq.bicS();
    if (!_frame) {
        seq.spr(0x2F, 0xAD);  // SPRD SB
        seq.spr(0xAD, 0xAE);  // SPRW PSR
        seq.spr(0xAD, 0xAF);  // SPRW MOD
    }
    if (_pins->fpu()) {
        seq.bytes({0x3E, 0x77, 0x05, STORE});  // SFSR @STORE
        for (uint_fast8_t n = 0; n < 8; ++n)
            seq.storeF(n);
    }
    seq.branch(_parkedAt, _parkedAt);
    uint8_t buf[96];
    auto org = _parkedAt;
    const auto cap =
            _pins->captureWrites(seq.buf, seq.len, buf, sizeof(buf), org, org);
    const uint_fast8_t want = 48 + (_frame ? 0 : 8) + (_pins->fpu() ? 36 : 0);
    if (cap < want) {
        cli.println("?save: short");
        return;
    }
    const uint8_t *p = buf;
    for (auto &r : _r) {
        r = le32(p);
        p += 4;
    }
    _sp0 = le32(p);
    _fp = le32(p + 4);
    _intbase = le32(p + 8);
    _sp1 = le32(p + 12);
    p += 16;
    if (_frame) {
        _sp0 += InstNs32016::FRAME_BYTES;
        _sb = _mems->read32(_mod);
    } else {
        _sb = le32(p);
        _psr = p[4] | (p[5] << 8);
        _mod = p[6] | (p[7] << 8);
        p += 8;
    }
    if (_pins->fpu()) {
        _fsr = le32(p);
        p += 4;
        for (auto &f : _f) {
            f = le32(p);
            p += 4;
        }
    }
}

// Set SP1 with S set and SP0 below a frame, then the rest, and RETT pops
// the frame, which the debugger answers with PC, MOD and PSR, and reloads
// SB from MOD, answered too. The CPU parks on the program's fetch.
void RegsNs32016::restore(bool trace) {
    const auto frame = _sp0 - InstNs32016::FRAME_BYTES;
    uint16_t psr = _psr & ~InstNs32016::PSR_P;
    if (trace)
        psr |= InstNs32016::PSR_T;
    Seq seq;
    seq.bicI();  // RETT reloads the program's I
    seq.bisS();
    seq.lprd(0xEF, 0xA4, _sp1);  // LPRD SP, #sp1
    seq.bicS();
    seq.lprd(0xEF, 0xA4, frame);     // LPRD SP, #frame
    seq.lprd(0x6F, 0xA4, _fp);       // LPRD FP, #fp
    seq.lprd(0x6F, 0xA7, _intbase);  // LPRD INTBASE, #intbase
    PinsNs32016::Keyed keys[6 + 16];
    uint_fast8_t nkeys = 0;
    const auto key = [&keys, &nkeys](uint32_t addr, uint16_t data) {
        keys[nkeys++] = PinsNs32016::Keyed{addr & InstNs32016::ADDR_MASK, data};
    };
    key(frame + 0, _pc);
    key(frame + 2, _pc >> 16);
    key(frame + 4, _mod);
    key(frame + 6, psr);
    key(_mod, _sb);
    key(_mod + 2, _sb >> 16);
    if (_pins->fpu()) {
        seq.bytes({0x3E, 0x0F, 0xA0});  // LFSR #fsr
        seq.be32(_fsr);
        for (uint_fast8_t n = 0; n < 8; ++n) {
            const auto at = FLOAD + 4 * n;
            seq.loadF(n, at);
            key(at, _f[n]);
            key(at + 2, _f[n] >> 16);
        }
    }
    for (uint_fast8_t n = 0; n < 8; ++n)
        seq.loadR(n, _r[n]);
    seq.bytes({InstNs32016::RETT, 0x00});
    auto org = _parkedAt;
    _pins->execInst(seq.buf, seq.len, org, _pc, keys, nkeys);
    _parkedAt = _pc;
    _frame = false;
    _tracing = trace;
}

void RegsNs32016::helpRegisters() const {
    cli.print("?Reg: PC PSR MOD SB FP SP0 SP1 INTBASE R0-R7");
    if (_pins->fpu())
        cli.print(" F0-F7 FSR");
    cli.println();
}

constexpr const char *REGS32[] = {
        "R0",
        "R1",
        "R2",
        "R3",
        "R4",
        "R5",
        "R6",
        "R7",  // 1-8
        "SB",
        "FP",
        "SP0",
        "SP1",
        "INTBASE",  // 9-13
};
constexpr const char *REGS24[] = {
        "PC",  // 14
};
constexpr const char *REGS16[] = {
        "PSR",
        "MOD",  // 15-16
};
constexpr const char *REGSF[] = {
        "F0",
        "F1",
        "F2",
        "F3",
        "F4",
        "F5",
        "F6",
        "F7",
        "FSR",  // 17-25
};

const Regs::RegList *RegsNs32016::listRegisters(uint_fast8_t n) const {
    static constexpr RegList REG_LIST[] = {
            {REGS32, 13, 1, UINT32_MAX},
            {REGS24, 1, 14, InstNs32016::ADDR_MASK},
            {REGS16, 2, 15, UINT16_MAX},
            {REGSF, 9, 17, UINT32_MAX},
    };
    const uint_fast8_t num = _pins->fpu() ? 4 : 3;
    return n < num ? &REG_LIST[n] : nullptr;
}

bool RegsNs32016::setRegister(uint_fast8_t reg, uint32_t value) {
    if (reg >= 1 && reg <= 8) {
        _r[reg - 1] = value;
    } else if (reg == 9) {
        _sb = value;
    } else if (reg == 10) {
        _fp = value;
    } else if (reg == 11) {
        _sp0 = value;
    } else if (reg == 12) {
        _sp1 = value;
    } else if (reg == 13) {
        _intbase = value;
    } else if (reg == 14) {
        _pc = value;
        return true;
    } else if (reg == 15) {
        _psr = value;
    } else if (reg == 16) {
        _mod = value;
    } else if (reg >= 17 && reg <= 24) {
        _f[reg - 17] = value;
    } else if (reg == 25) {
        _fsr = value;
    }
    return false;
}

}  // namespace ns32016
}  // namespace debugger

// Local Variables:
// mode: c++
// c-basic-offset: 4
// tab-width: 4
// End:
// vim: set ft=cpp et ts=4 sw=4:
