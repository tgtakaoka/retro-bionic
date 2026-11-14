#include "regs_mc68000.h"

#include "debugger.h"
#include "inst_mc68000.h"
#include "mems_mc68000.h"
#include "pins_mc68000_base.h"
#include "save_mc68000.h"

namespace debugger {
namespace mc68000 {
namespace {
// clang-format off
//                              1         2         3         4
//                    012345678901234567890123456789012345678901234567
const char line1[] = "PC=xxxxxxxx SR=T_S_x   CCR=XNZVC   USP=xxxxxxxx";
const char lineD0[] = "D0=xxxxxxxx D1=xxxxxxxx D2=xxxxxxxx D3=xxxxxxxx";
const char lineD4[] = "D4=xxxxxxxx D5=xxxxxxxx D6=xxxxxxxx D7=xxxxxxxx";
const char lineA0[] = "A0=xxxxxxxx A1=xxxxxxxx A2=xxxxxxxx A3=xxxxxxxx";
const char lineA4[] = "A4=xxxxxxxx A5=xxxxxxxx A6=xxxxxxxx SP=xxxxxxxx";
const char line10[] = "VBR=xxxxxxxx SFC=x DFC=x";
// clang-format on

// The restore's instructions, built word by word.
struct Sequence {
    uint8_t buf[3 * (6 + 4) + 6 + 2 + 6 * 8 + 6 * 8 + 2];
    uint_fast8_t len = 0;

    void word(uint16_t v) {
        buf[len++] = v >> 8;
        buf[len++] = v;
    }
    void immediate(uint16_t opcode, uint32_t v) {
        word(opcode);
        word(v >> 16);
        word(v);
    }
};
}  // namespace

RegsMc68000::RegsMc68000(PinsMc68000Base *pins, MemsMc68000 *mems, bool mc68008)
    : _pins(pins),
      _mems(mems),
      _mc68008(mc68008),
      _pc(InstMc68000::ORG_PARK),
      _parkedAddr(InstMc68000::ORG_PARK),
      _vecAddr(0),
      _sr(InstMc68000::SR_RESET),
      _ssp(0),
      _usp(0),
      _d{0, 0, 0, 0, 0, 0, 0, 0},
      _a{0, 0, 0, 0, 0, 0, 0},
      _vbr(0),
      _sfc(0),
      _dfc(0) {}

uint32_t RegsMc68000::vecAddr(uint8_t vecNum) const {
    return _mems->wrap(_vbr + InstMc68000::vecAddr(vecNum));
}

// A7 is SP, the stack pointer of the mode SR is in; the other one shows
// on the first line, its value above SP's.
void RegsMc68000::print() const {
    const auto super = (_sr & InstMc68000::SR_S) != 0;
    CharBuffer b1(line1);
    b1.hex32(3, _pc);
    // Digits above the board's address lines don't exist.
    const auto digits = (32 - __builtin_clz(_mems->maxAddr()) + 3) / 4;
    for (auto i = 0; i < 8 - digits; ++i)
        b1[3 + i] = '_';
    b1.bits(15, _sr, InstMc68000::SR_T, "T_S_");
    b1.hex4(19, (_sr >> 8) & 7);
    b1.bits(27, _sr, 0x10, "XNZVC");
    b1[35] = super ? 'U' : 'S';
    b1.hex32(39, super ? _usp : _ssp);
    cli.println(b1);
    _pins->idle();
    const uint32_t *regs[] = {&_d[0], &_d[4], &_a[0], &_a[4]};
    const char *const lines[] = {lineD0, lineD4, lineA0, lineA4};
    for (uint_fast8_t l = 0; l < 4; ++l) {
        CharBuffer b(lines[l]);
        for (uint_fast8_t i = 0; i < 4; ++i) {
            const auto v = (l == 3 && i == 3) ? (super ? _ssp : _usp)
                                              : regs[l][i];
            b.hex32(3 + 12 * i, v);
        }
        cli.println(b);
        _pins->idle();
    }
}

// The MC68010's VBR, SFC and DFC, which a step leaves alone.
void RegsMc68000::printAll() const {
    print();
    if (_mc68010) {
        CharBuffer b(line10);
        b.hex32(4, _vbr);
        b.hex4(17, _sfc);
        b.hex4(23, _dfc);
        cli.println(b);
        _pins->idle();
    }
}

// The pushes of SaveMc68000. SR is the program's only when the CPU was
// parked at its own fetch, after reset; otherwise the exception frame
// holds it.
void RegsMc68000::save() {
    if (_vector && !_pins->answerVector(_vecAddr, InstMc68000::ORG_PARK)) {
        cli.println("?save: vector not answered");
        return;
    }
    auto org = _vector ? InstMc68000::ORG_PARK : _parkedAddr;
    const auto save = _mc68010 ? SaveMc68000::SAVE10 : SaveMc68000::SAVE;
    const auto len =
            _mc68010 ? sizeof(SaveMc68000::SAVE10) : sizeof(SaveMc68000::SAVE);
    const auto bytes = SaveMc68000::bytes(_mc68010);
    uint8_t buf[SaveMc68000::MAX_BYTES];
    uint32_t addrs[SaveMc68000::MAX_BYTES];
    const auto n = _pins->captureWrites(
            save, len, buf, addrs, bytes, org, PinsMc68000Base::EXIT_ORG);
    SaveMc68000::Pushed p;
    if (n < bytes || _pins->cutShort() ||
            !SaveMc68000::decode(
                    buf, addrs, n, _mems->maxAddr(), _mc68010, p)) {
        cli.println("?save: registers not pushed");
        return;
    }
    for (uint_fast8_t i = 0; i < 8; ++i)
        _d[i] = p.d[i];
    for (uint_fast8_t i = 0; i < 7; ++i)
        _a[i] = p.a[i];
    _usp = p.usp;
    if (_mc68010) {
        _vbr = p.vbr;
        _sfc = p.sfc;
        _dfc = p.dfc;
    }
    if (!_vector) {
        // Parked at the program's own fetch: the push was its SSP and SR.
        _pc = _parkedAddr;
        _ssp = p.sp;
        _sr = p.sr;
    }
    // Either way the CPU is parked at |org| again, outside a vector.
    _vector = false;
    _parkedAddr = org;
}

void RegsMc68000::reset() {
    _ssp = _mems->read32(InstMc68000::VEC_RESET);
    _pc = _mems->wrap(_mems->read32(InstMc68000::VEC_RESET + 4)) & ~1;
    _sr = InstMc68000::SR_RESET;
    _vbr = 0;
}

// Every register comes back as an immediate, the MC68010's VBR, SFC and
// DFC and USP through A0 first; then
// A7 points at a frame staged just below SSP, which RTE pulls as real
// memory. Exceptions write there anyway, so it is free stack. The
// MC68010's RTE reads a format/vector word too: format 0, a short frame.
void RegsMc68000::restore(bool trace) {
    if (_vector && !_pins->answerVector(_vecAddr, InstMc68000::ORG_PARK)) {
        cli.println("?restore: vector not answered");
        return;
    }
    const auto frame = _mems->wrap(_ssp - frameBytes());
    const auto sr = trace ? (_sr | InstMc68000::SR_T) : _sr;
    const auto stageWord = [this](uint32_t addr, uint16_t v) {
        _mems->write_byte(_mems->wrap(addr), v >> 8);
        _mems->write_byte(_mems->wrap(addr + 1), v & 0xFF);
    };
    stageWord(frame, sr);
    stageWord(frame + 2, _pc >> 16);
    stageWord(frame + 4, _pc);
    if (_mc68010)
        stageWord(frame + 6, 0x0000);
    // MOVEA.L #,A0 and MOVEC A0,Rc for VBR, SFC, DFC; MOVEA.L #usp,A0;
    // MOVE A0,USP; MOVE.L #Dn x8; MOVEA.L #An x7, A7 last; RTE.
    Sequence seq;
    constexpr uint16_t MOVE_L_IMM_D = 0x203C;   // MOVE.L #,Dn: n in bits 11-9
    constexpr uint16_t MOVEA_L_IMM_A = 0x207C;  // MOVEA.L #,An
    constexpr uint16_t MOVE_A0_USP = 0x4E60;
    constexpr uint16_t MOVEC_TO_RC = 0x4E7B;  // MOVEC Rn,Rc
    constexpr uint16_t A0_VBR = 0x8801, A0_SFC = 0x8000, A0_DFC = 0x8001;
    if (_mc68010) {
        seq.immediate(MOVEA_L_IMM_A, _vbr);
        seq.word(MOVEC_TO_RC);
        seq.word(A0_VBR);
        seq.immediate(MOVEA_L_IMM_A, _sfc);
        seq.word(MOVEC_TO_RC);
        seq.word(A0_SFC);
        seq.immediate(MOVEA_L_IMM_A, _dfc);
        seq.word(MOVEC_TO_RC);
        seq.word(A0_DFC);
    }
    seq.immediate(MOVEA_L_IMM_A, _usp);
    seq.word(MOVE_A0_USP);
    for (uint_fast8_t i = 0; i < 8; ++i)
        seq.immediate(MOVE_L_IMM_D | i << 9, _d[i]);
    for (uint_fast8_t i = 0; i < 7; ++i)
        seq.immediate(MOVEA_L_IMM_A | i << 9, _a[i]);
    seq.immediate(MOVEA_L_IMM_A | 7 << 9, frame);
    seq.word(InstMc68000::RTE);
    auto org = _vector ? InstMc68000::ORG_PARK : _parkedAddr;
    _pins->execInst(seq.buf, seq.len, org, _pc);
    _vector = false;
    _parkedAddr = org;  // the fetch of the PC, as the bus showed it
}

void RegsMc68000::helpRegisters() const {
    cli.print("?Reg: PC SR CCR D0-D7 A0-A6 SSP USP");
    cli.println(_mc68010 ? " VBR SFC DFC" : "");
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
        "D0",   // 4
        "D1",   // 5
        "D2",   // 6
        "D3",   // 7
        "D4",   // 8
        "D5",   // 9
        "D6",   // 10
        "D7",   // 11
        "A0",   // 12
        "A1",   // 13
        "A2",   // 14
        "A3",   // 15
        "A4",   // 16
        "A5",   // 17
        "A6",   // 18
        "SSP",  // 19
        "USP",  // 20
        "VBR",  // 21
};
constexpr const char *REGS3[] = {
        "SFC",  // 22
        "DFC",  // 23
};
}  // namespace

const Regs::RegList *RegsMc68000::listRegisters(uint_fast8_t n) const {
    static constexpr RegList REG_LIST[] = {
            {REGS8, 1, 1, UINT8_MAX},
            {REGS16, 1, 2, UINT16_MAX},
            {REGS24, 1, 3, 0xFFFFFF},
            {REGS32, 17, 4, UINT32_MAX},
            {REGS32, 18, 4, UINT32_MAX},
            {REGS3, 2, 22, 7},
    };
    // The MC68010's list of longs takes VBR too.
    static constexpr uint8_t LISTS[] = {0, 1, 2, 3};
    static constexpr uint8_t LISTS10[] = {0, 1, 2, 4, 5};
    if (_mc68010)
        return n < sizeof(LISTS10) ? &REG_LIST[LISTS10[n]] : nullptr;
    return n < sizeof(LISTS) ? &REG_LIST[LISTS[n]] : nullptr;
}

bool RegsMc68000::setRegister(uint_fast8_t reg, uint32_t value) {
    if (reg == 1) {
        _sr = (_sr & 0xFF00) | (value & 0x1F);
    } else if (reg == 2) {
        _sr = value;
    } else if (reg == 3) {
        // Instructions are words: PC stays even.
        _pc = _mems->wrap(value) & ~1;
        return true;
    } else if (reg >= 4 && reg < 12) {
        _d[reg - 4] = value;
    } else if (reg >= 12 && reg < 19) {
        _a[reg - 12] = value;
    } else if (reg == 19) {
        _ssp = value;
    } else if (reg == 20) {
        _usp = value;
    } else if (_mc68010 && reg == 21) {
        _vbr = value;
    } else if (_mc68010 && reg == 22) {
        _sfc = value & 7;
    } else if (_mc68010 && reg == 23) {
        _dfc = value & 7;
    }
    return false;
}

}  // namespace mc68000
}  // namespace debugger

// Local Variables:
// mode: c++
// c-basic-offset: 4
// tab-width: 4
// End:
// vim: set ft=cpp et ts=4 sw=4:
