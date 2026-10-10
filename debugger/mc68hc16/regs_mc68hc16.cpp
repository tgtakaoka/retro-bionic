#include "regs_mc68hc16.h"

#include <initializer_list>
#include "debugger.h"
#include "inst_mc68hc16.h"
#include "mems_mc68hc16.h"
#include "pins_mc68hc16.h"

namespace debugger {
namespace mc68hc16 {
namespace {
// clang-format off
//                              1         2         3         4
//                    01234567890123456789012345678901234567890123456789
const char line1[] = "PC=x:xxxx SP=x:xxxx CCR=SMHENZVC IP=x SM=x";
const char line2[] = "D=xxxx E=xxxx IX=x:xxxx IY=x:xxxx IZ=x:xxxx EK=x";
const char line3[] = "H=xxxx I=xxxx AM=x:xxxxxxxx SL=x XM=xx YM=xx";
// clang-format on

constexpr uint32_t ADDR_MASK = MemsMc68hc16::ADDR_MASK;

constexpr uint32_t wrap(uint32_t addr) {
    return addr & ADDR_MASK;
}

// Captured bytes by address: the words a push stored, whatever the SP's
// alignment split them into.
struct Captured {
    const uint8_t *buf;
    const uint32_t *addrs;
    uint_fast8_t from;
    uint_fast8_t to;

    bool byte(uint32_t addr, uint8_t &v) const {
        for (auto i = from; i < to; ++i) {
            if (addrs[i] == wrap(addr)) {
                v = buf[i];
                return true;
            }
        }
        return false;
    }
    bool word(uint32_t addr, uint16_t &v) const {
        uint8_t h, l;
        if (!byte(addr, h) || !byte(addr + 1, l))
            return false;
        v = static_cast<uint16_t>(h) << 8 | l;
        return true;
    }
    // The address of the word pushed first: the highest of the push.
    uint32_t top() const {
        uint32_t hi = 0;
        for (auto i = from; i < to; ++i) {
            if (addrs[i] > hi)
                hi = addrs[i];
        }
        return wrap(hi - 1);
    }
};
}  // namespace

RegsMc68hc16::RegsMc68hc16(PinsMc68hc16 *pins, MemsMc68hc16 *mems)
    : _pins(pins),
      _mems(mems),
      _pc(InstMc68hc16::ORG_PARK),
      _parkedAddr(InstMc68hc16::ORG_PARK),
      _vecAddr(0),
      _sp(0),
      _ccr(0),
      _d(0),
      _e(0),
      _ix(0),
      _iy(0),
      _iz(0),
      _k(0),
      _mac{0, 0, 0, 0, 0, 0},
      _buffer1(line1),
      _buffer2(line2),
      _buffer3(line3) {}

void RegsMc68hc16::print() const {
    auto b1 = _buffer1;
    b1.hex4(3, _pc >> 16);
    b1.hex16(5, _pc);
    b1.hex4(13, _sp >> 16);
    b1.hex16(15, _sp);
    b1.bits(24, _ccr >> 8, 0x80, line1 + 24);
    b1.hex4(36, (_ccr >> 5) & 7);
    b1.hex4(41, (_ccr >> 4) & 1);
    cli.println(b1);
    _pins->idle();
    auto b2 = _buffer2;
    b2.hex16(2, _d);
    b2.hex16(9, _e);
    b2.hex4(17, xk());
    b2.hex16(19, _ix);
    b2.hex4(27, yk());
    b2.hex16(29, _iy);
    b2.hex4(37, zk());
    b2.hex16(39, _iz);
    b2.hex4(47, ek());
    cli.println(b2);
    _pins->idle();
    auto b3 = _buffer3;
    b3.hex16(2, _mac[0]);
    b3.hex16(9, _mac[1]);
    b3.hex4(17, _mac[4] & 0xF);
    b3.hex16(19, _mac[3]);
    b3.hex16(23, _mac[2]);
    b3.hex4(31, _mac[4] >> 15);
    b3.hex8(36, _mac[5] >> 8);
    b3.hex8(42, _mac[5]);
    cli.println(b3);
    _pins->idle();
}

uint32_t RegsMc68hc16::fingerprint() const {
    uint32_t h = 0;
    for (const uint32_t v :
            {_pc, _sp, uint32_t(_ccr), uint32_t(_d), uint32_t(_e),
                    uint32_t(_ix), uint32_t(_iy), uint32_t(_iz), uint32_t(_k)})
        h = (h << 5 | h >> 27) ^ v;
    return h;
}

// PSHM stacks D first and CCR last, a word each, from SK:SP down; PSHMAC
// stacks the MAC unit below that. BRA back to the origin fetches it again
// once both have run.
void RegsMc68hc16::save() {
    // clang-format off
    static constexpr uint8_t SAVE[] = {
        0x34, 0x7F,  // PSHM D,E,X,Y,Z,K,CCR
        0x27, 0xB8,  // PSHMAC
        0xB0, 0xF6,  // BRA  org
    };
    // clang-format on
    static_assert(SAVE[sizeof(SAVE) - 1] == uint8_t(-(sizeof(SAVE) + 4)),
            "BRA org: the displacement is from the BRA plus 6");
    constexpr uint_fast8_t PSHM_BYTES = 14;
    constexpr uint_fast8_t PSHMAC_BYTES = 12;
    if (_vector != NONE &&
            !_pins->answerVector(_vecAddr, InstMc68hc16::ORG_PARK)) {
        cli.println("?save: vector not answered");
        return;
    }
    auto org = _vector != NONE ? InstMc68hc16::ORG_PARK : _parkedAddr;
    uint8_t buf[PSHM_BYTES + PSHMAC_BYTES];
    uint32_t addrs[sizeof(buf)];
    const auto n = _pins->captureWrites(SAVE, sizeof(SAVE), buf, addrs,
            sizeof(buf), org, PinsMc68hc16::EXIT_ORG);
    if (n < sizeof(buf) || _pins->cutShort()) {
        cli.println("?save: registers not pushed");
        return;
    }
    const Captured pshm{buf, addrs, 0, PSHM_BYTES};
    const Captured pshmac{buf, addrs, PSHM_BYTES, sizeof(buf)};
    const auto top = pshm.top();
    uint16_t ccr = _ccr;
    pshm.word(top, _d);
    pshm.word(top - 2, _e);
    pshm.word(top - 4, _ix);
    pshm.word(top - 6, _iy);
    pshm.word(top - 8, _iz);
    pshm.word(top - 10, _k);
    pshm.word(top - 12, ccr);
    const auto mac = pshmac.top();
    for (uint_fast8_t i = 0; i < 6; ++i)
        pshmac.word(mac - 2 * i, _mac[i]);
    if (_vector == NONE) {
        // Parked at the program's own fetch: the push was its SP.
        _pc = _parkedAddr;
        _sp = top;
        _ccr = ccr;
    }
    // Either way the CPU is parked at |org| again, outside a vector.
    _vector = NONE;
    _parkedAddr = org;
}

void RegsMc68hc16::reset() {
    // The program's reset vector: ZK:SK:PK, PC, SP and IZ (CPU16RM
    // Figure 9-3).
    const auto kk = _mems->read16(InstMc68hc16::VEC_RESET);
    const auto pk = kk & 0xF;
    _pc = static_cast<uint32_t>(pk) << 16 | _mems->read16(2);
    _sp = static_cast<uint32_t>((kk >> 4) & 0xF) << 16 | _mems->read16(4);
    _iz = _mems->read16(6);
    _k = ((kk >> 8) & 0xF) << 4;
    // Reset sets S and the interrupt mask, and clears SM.
    _ccr = (_ccr & 0x7F00) | 0x80E0 | pk;
}

// Everything comes back from a frame staged just below SP: PULMAC and
// PULM read it as real memory, and RTI the CCR and PC at SP itself.
// Exceptions write there anyway, so it is free stack.
void RegsMc68hc16::restore() {
    if (_vector != NONE &&
            !_pins->answerVector(_vecAddr, InstMc68hc16::ORG_PARK)) {
        cli.println("?restore: vector not answered");
        return;
    }
    const auto put = [this](uint32_t addr, uint16_t v) {
        _mems->write_byte(wrap(addr), v >> 8);
        _mems->write_byte(wrap(addr + 1), v & 0xFF);
    };
    // RTI subtracts 6 from the PK:PC it pulls.
    const auto stacked = wrap(_pc + InstMc68hc16::IRQ_PC_OFFSET);
    const auto p = _sp;
    put(p - 2, (_ccr & ~0xF) | (stacked >> 16));
    put(p, stacked);
    const auto r = p - 4;  // PULM's last word, D
    put(r, _d);
    put(r - 2, _e);
    put(r - 4, _ix);
    put(r - 6, _iy);
    put(r - 8, _iz);
    put(r - 10, _k);
    const auto mac = r - 12;  // PSHMAC's SP: the MAC words go back there
    for (uint_fast8_t i = 0; i < 6; ++i)
        put(mac - 2 * i, _mac[i]);
    const auto s = wrap(mac - 12);  // SK:SP for PULMAC
    // clang-format off
    const uint8_t RESTORE[] = {
        0xF5, uint8_t((s >> 16) & 0xF),          // LDAB #sk
        0x37, 0x9F,                              // TBSK
        0x37, 0xBF, uint8_t(s >> 8), uint8_t(s), // LDS  #sp
        0x27, 0xB9,                              // PULMAC
        0x35, 0x7E,                              // PULM D,E,X,Y,Z,K
        0x27, 0x77,                              // RTI
    };
    // clang-format on
    auto org = _vector != NONE ? InstMc68hc16::ORG_PARK : _parkedAddr;
    _pins->execInst(RESTORE, sizeof(RESTORE), org, _pc);
    _vector = NONE;
    _parkedAddr = org;  // the fetch of the PC, as the bus showed it
}

void RegsMc68hc16::helpRegisters() const {
    cli.println("?Reg: PC SP CCR D A B E IX IY IZ K XK YK ZK EK SK PK");
    cli.println("      H I");
}

namespace {
constexpr const char *REGS8[] = {
        "A",  // 1
        "B",  // 2
};
constexpr const char *REGS4[] = {
        "XK",  // 3
        "YK",  // 4
        "ZK",  // 5
        "EK",  // 6
        "SK",  // 7
        "PK",  // 8
};
constexpr const char *REGS16[] = {
        "D",    // 9
        "E",    // 10
        "IX",   // 11
        "IY",   // 12
        "IZ",   // 13
        "CCR",  // 14
        "K",    // 15
        "H",    // 16
        "I",    // 17
};
constexpr const char *REGS20[] = {
        "PC",  // 18
        "SP",  // 19
};
}  // namespace

const Regs::RegList *RegsMc68hc16::listRegisters(uint_fast8_t n) const {
    static constexpr RegList REG_LIST[] = {
            {REGS8, 2, 1, UINT8_MAX},
            {REGS4, 6, 3, 0xF},
            {REGS16, 9, 9, UINT16_MAX},
            {REGS20, 2, 18, ADDR_MASK},
    };
    return n < sizeof(REG_LIST) / sizeof(REG_LIST[0]) ? &REG_LIST[n] : nullptr;
}

bool RegsMc68hc16::setRegister(uint_fast8_t reg, uint32_t value) {
    switch (reg) {
    case 1:
        _d = (_d & 0x00FF) | (value << 8);
        break;
    case 2:
        _d = (_d & 0xFF00) | (value & 0xFF);
        break;
    case 3:
        setK(12, value);
        break;
    case 4:
        setK(8, value);
        break;
    case 5:
        setK(4, value);
        break;
    case 6:
        setK(0, value);
        break;
    case 7:
        _sp = (value & 0xF) << 16 | (_sp & 0xFFFF);
        break;
    case 8:
        // Instructions are words: PK:PC stays even.
        _pc = (value & 0xF) << 16 | (_pc & 0xFFFE);
        _ccr = (_ccr & ~0xF) | (value & 0xF);
        return true;
    case 9:
        _d = value;
        break;
    case 10:
        _e = value;
        break;
    case 11:
        _ix = value;
        break;
    case 12:
        _iy = value;
        break;
    case 13:
        _iz = value;
        break;
    case 14:
        _ccr = (value & ~0xF) | ((_pc >> 16) & 0xF);
        break;
    case 15:
        _k = value;
        break;
    case 16:
        _mac[0] = value;
        break;
    case 17:
        _mac[1] = value;
        break;
    case 18:
        _pc = value & (ADDR_MASK & ~1);
        _ccr = (_ccr & ~0xF) | ((_pc >> 16) & 0xF);
        return true;
    case 19:
        _sp = value;
        break;
    }
    return false;
}

}  // namespace mc68hc16
}  // namespace debugger

// Local Variables:
// mode: c++
// c-basic-offset: 4
// tab-width: 4
// End:
// vim: set ft=cpp et ts=4 sw=4:
