#include "regs_z380.h"

#include <initializer_list>
#include "debugger.h"
#include "inst_z380.h"
#include "mems_z380.h"
#include "pins_z380.h"

namespace debugger {
namespace z380 {
namespace {
// clang-format off
//                              1         2         3         4         5         6         7
//                    01234567890123456789012345678901234567890123456789012345678901234567890
const char line1[] = "     PC=xxxxxxxx SP=xxxxxxxx SR=Y0_X0_M0_______";
const char line2[] = "A=xx BC=xxxxxxxx DE=xxxxxxxx HL=xxxxxxxx IX=xxxxxxxx IY=xxxxxxxx";
const char line3[] = "A'xx BC'xxxxxxxx DE'xxxxxxxx HL'xxxxxxxx IX'xxxxxxxx IY'xxxxxxxx";
const char line4[] = "F=SZ1H1VNC F'SZ1H1VNC I=xxxxxxxx R=xx";
// clang-format on

constexpr uint8_t lo8(uint32_t v) {
    return static_cast<uint8_t>(v);
}
constexpr uint8_t hi8(uint32_t v) {
    return static_cast<uint8_t>(v >> 8);
}
constexpr uint8_t lo8z(uint32_t v) {
    return static_cast<uint8_t>(v >> 16);
}
constexpr uint8_t hi8z(uint32_t v) {
    return static_cast<uint8_t>(v >> 24);
}
constexpr uint32_t le32(const uint8_t *lo, const uint8_t *hi) {
    return static_cast<uint32_t>(hi[1]) << 24 |
           static_cast<uint32_t>(hi[0]) << 16 |
           static_cast<uint32_t>(lo[1]) << 8 | lo[0];
}

// DDIR IW,LW: the next instruction takes a 32-bit immediate.
constexpr uint8_t DDIR_IW_LW_0 = 0xFD;
constexpr uint8_t DDIR_IW_LW_1 = 0xC2;

// A sequence that runs off its end must end on a word boundary: the read
// that ends it is a word fetch at an even address.
#define ASSERT_EVEN(seq) static_assert(sizeof(seq) % 2 == 0, #seq " is odd")
}  // namespace

RegsZ380::RegsZ380(PinsZ380 *pins, MemsZ380 *mems)
    : _pins(pins),
      _mems(mems),
      _buffer1(line1),
      _buffer2(line2),
      _buffer3(line3),
      _buffer4(line4) {}

// Only the set the SR selects; the other banks are reached through '='.
void RegsZ380::print() const {
    const auto &bank = live();
    const auto a = afp(_sr), b = alt(_sr);
    auto b2 = _buffer2;
    b2.hex8(2, bank.a[a]);
    b2.hex32(8, bank.bc[b]);
    b2.hex32(20, bank.de[b]);
    b2.hex32(32, bank.hl[b]);
    b2.hex32(44, ix());
    b2.hex32(56, iy());
    cli.println(b2);
    _pins->idle();
    auto b3 = _buffer3;
    b3.hex8(2, bank.a[a ^ 1]);
    b3.hex32(8, bank.bc[b ^ 1]);
    b3.hex32(20, bank.de[b ^ 1]);
    b3.hex32(32, bank.hl[b ^ 1]);
    b3.hex32(44, ix(1));
    b3.hex32(56, iy(1));
    cli.println(b3);
    _pins->idle();
    auto b1 = _buffer1;
    b1.hex32(8, _pc);
    b1.hex32(20, _sp);
    // SR, one character per field: banks with ' for the prime sides,
    // then XM LW IEF1 IM LCK AFP, _ where clear.
    const auto flag = [&](uint_fast8_t pos, uint32_t bit, char c) {
        b1[pos] = (_sr & bit) ? c : '_';
    };
    b1[33] = '0' + iyBank(_sr);
    flag(34, UINT32_C(1) << 24, '\'');
    b1[36] = '0' + ixBank(_sr);
    flag(37, UINT32_C(1) << 16, '\'');
    b1[39] = '0' + mainBank(_sr);
    flag(40, UINT32_C(1) << 8, '\'');
    flag(41, SR_XM, 'X');
    flag(42, SR_LW, 'L');
    flag(43, 0x20, 'I');
    b1[44] = '0' + ((_sr >> 3) & 3);
    flag(45, 0x02, 'L');
    flag(46, 0x01, '\'');
    cli.println(b1);
    _pins->idle();
    auto b4 = _buffer4;
    b4.bits(2, bank.f[a], 0x80, b4 + 2);
    b4.bits(13, bank.f[a ^ 1], 0x80, b4 + 13);
    b4.hex32(24, _i);
    b4.hex8(35, _r);
    cli.println(b4);
    _pins->idle();
}

// JP |addr|, with DDIR IW for a 32-bit target in Extended mode.
uint_fast8_t RegsZ380::jumpTo(uint8_t *buf, uint32_t addr) const {
    uint_fast8_t len = 0;
    if (extended()) {
        buf[len++] = 0xFD;  // DDIR IW
        buf[len++] = 0xC3;
    }
    buf[len++] = InstZ380::JP;
    buf[len++] = lo8(addr);
    buf[len++] = hi8(addr);
    if (extended()) {
        buf[len++] = lo8z(addr);
        buf[len++] = hi8z(addr);
    }
    return len;
}

uint32_t RegsZ380::fingerprint() const {
    const auto &bank = live();
    const auto a = afp(_sr), b = alt(_sr);
    uint32_t h = 0;
    for (const uint32_t v : {_pc, _sp, uint32_t(bank.a[a]) << 8 | bank.f[a],
                 bank.bc[b], bank.de[b], bank.hl[b], ix(), iy()})
        h = (h << 5 | h >> 27) ^ v;
    return h;
}

void RegsZ380::selectFrame() {
    const auto &f = _frames[extended() ? (_frames[1].valid ? 1 : 0)
                                       : (_frames[0].valid ? 0 : 1)];
    _pc = f.pc;
    _frameAddr = f.addr;
}

void RegsZ380::updateMode() const {
    _mems->setMode(extended(), (_sr & SR_LW) != 0);
}

// One bank, both sides. Word mode (save() clears LW), so every PUSH is one
// 16-bit write; SWAP brings the "z" extension down and puts it back. The
// prefetch reaches the end of the window before the last write lands, so
// the sequence ends in a jump that fetches the exit again after it.
void RegsZ380::saveBank(uint_fast8_t bank, uint32_t &org) {
    const uint8_t dsr = bank << 1;
    // clang-format off
    const uint8_t SAVE_BANK[] = {
        0xED, 0xDA, dsr,        // LDCTL DSR, bank   ; primary BC/DE/HL
        0xF5,                   // PUSH AF           ; the side AFP selects
        0xC5, 0xED, 0x0E,       // PUSH BC; SWAP BC
        0xC5, 0xED, 0x0E,       // PUSH BC; SWAP BC
        0xD5, 0xED, 0x1E,       // PUSH DE; SWAP DE
        0xD5, 0xED, 0x1E,       // PUSH DE; SWAP DE
        0xE5, 0xED, 0x3E,       // PUSH HL; SWAP HL
        0xE5, 0xED, 0x3E,       // PUSH HL; SWAP HL
        0x08, 0xF5, 0x08,       // EX AF,AF'; PUSH AF; EX AF,AF'
        0xED, 0xDA, uint8_t(dsr | 1),  // LDCTL DSR, bank|ALT
        0xC5, 0xED, 0x0E,       // PUSH BC; SWAP BC
        0xC5, 0xED, 0x0E,       // PUSH BC; SWAP BC
        0xD5, 0xED, 0x1E,       // PUSH DE; SWAP DE
        0xD5, 0xED, 0x1E,       // PUSH DE; SWAP DE
        0xE5, 0xED, 0x3E,       // PUSH HL; SWAP HL
        0xE5, 0xED, 0x3E,       // PUSH HL; SWAP HL
        0x18, 0x00,             // JR $+2
    };
    // clang-format on
    ASSERT_EVEN(SAVE_BANK);
    uint8_t buf[28];
    _pins->captureWrites(SAVE_BANK, sizeof(SAVE_BANK), buf, sizeof(buf), org);
    auto &r = _bank[bank];
    const auto a = afp(_srCpu);
    r.f[a] = buf[0], r.a[a] = buf[1];
    r.bc[0] = le32(buf + 2, buf + 4);
    r.de[0] = le32(buf + 6, buf + 8);
    r.hl[0] = le32(buf + 10, buf + 12);
    r.f[a ^ 1] = buf[14], r.a[a ^ 1] = buf[15];
    r.bc[1] = le32(buf + 16, buf + 18);
    r.de[1] = le32(buf + 20, buf + 22);
    r.hl[1] = le32(buf + 24, buf + 26);
}

// IX or IY (|prefix| DD or FD) in every bank and side, through XSR or YSR.
void RegsZ380::saveIndex(uint32_t (&reg)[4][2], uint8_t prefix, uint32_t &org) {
    uint8_t seq[96];
    uint_fast8_t len = 0;
    const auto put = [&](std::initializer_list<uint8_t> bytes) {
        for (auto b : bytes)
            seq[len++] = b;
    };
    for (uint8_t n = 0; n < 8; ++n) {
        // LDCTL XSR/YSR, bank<<1|prime; PUSH; SWAP; PUSH; SWAP
        put({prefix, 0xDA, n, prefix, 0xE5, prefix, 0x3E, prefix, 0xE5, prefix,
                0x3E});
    }
    put({0x18, 0x00});  // JR $+2
    uint8_t buf[32];
    _pins->captureWrites(seq, len, buf, sizeof(buf), org);
    for (uint_fast8_t n = 0; n < 8; ++n)
        reg[n >> 1][n & 1] = le32(buf + n * 4, buf + n * 4 + 2);
}

void RegsZ380::save() {
    // HL and the Select Register first, each 16-bit half pushed as a word
    // under DDIR W, since the program may have left Long Word mode on;
    // then RESC LW so the rest runs in Word mode. restore() puts the SR
    // back last. None of these touches the flags.
    // clang-format off
    static constexpr uint8_t ENTRY[] = {
        0xDD, 0xC0, 0xE5,  // DDIR W; PUSH HL  ; captured
        0xED, 0x3E,        // SWAP HL
        0xDD, 0xC0, 0xE5,  // DDIR W; PUSH HL  ; captured
        0xFD, 0xC0,        // DDIR LW
        0xED, 0xC0,        // LDCTL HL, SR     ; all 32 bits
        0xDD, 0xC0, 0xE5,  // DDIR W; PUSH HL  ; captured
        0xED, 0x3E,        // SWAP HL
        0xDD, 0xC0, 0xE5,  // DDIR W; PUSH HL  ; captured
        0xDD, 0xFF,        // RESC LW
        0x18, 0x00,        // JR $+2
    };
    // CALL pushes the resume point, the origin plus its length: 2 bytes
    // in Native mode, 4 in Extended mode.
    static constexpr uint8_t PUSH_PC[] = {
        0xCD, 0x00, 0x80,  // CALL 8000H
    };
    // clang-format on
    ASSERT_EVEN(ENTRY);
    uint8_t buffer[8];
    const auto parked = parkedAt();
    auto org = parked;
    _pins->captureWrites(ENTRY, sizeof(ENTRY), buffer, 8, org);
    const auto hl = le32(buffer + 0, buffer + 2);
    _sr = _srCpu = le32(buffer + 4, buffer + 6);
    if (_vector != NONE)
        selectFrame();  // now that XM is known
    const auto pcSize = this->pcSize();
    const auto addr = _pins->captureWrites(
            PUSH_PC, sizeof(PUSH_PC), buffer, pcSize, org, 0x8000);
    switch (_vector) {
    case NONE: {
        // The four pushes of ENTRY moved SP too.
        _sp = spPlus(addr, pcSize + 8);
        // Extended mode pushes two words, the low one first as measured;
        // still, tell them apart by where the first one went.
        const auto lowFirst = _pins->firstWrite() == addr;
        const uint32_t pushed = pcSize == 2 ? le16(buffer)
                                : lowFirst  ? le32(buffer, buffer + 2)
                                            : le32(buffer + 2, buffer);
        _pc = pushed - sizeof(PUSH_PC) - sizeof(ENTRY);
        if (!extended())
            _pc &= UINT16_MAX;
        break;
    }
    case RST:
    case NMI:
        _sp = spPlus(_frameAddr, pcSize);  // above the pushed PC
        break;
    }
    for (uint_fast8_t bank = 0; bank < 4; ++bank)
        saveBank(bank, org);
    live().hl[alt(_sr)] = hl;  // ENTRY used it
    saveIndex(_ix, 0xDD, org);
    saveIndex(_iy, 0xFD, org);
    // The selections back as they were, then I and R. LD A,R has no bus
    // transfer of its own, so the value is read back through a captured
    // LD (HL),A.
    // clang-format off
    const uint8_t SAVE_OTHERS[] = {
        0xED, 0xDA, uint8_t(_srCpu >> 8),   // LDCTL DSR, dsr
        0xDD, 0xDA, uint8_t(_srCpu >> 16),  // LDCTL XSR, xsr
        0xFD, 0xDA, uint8_t(_srCpu >> 24),  // LDCTL YSR, ysr
        0xFD, 0xC0,  // DDIR LW
        0xDD, 0x57,  // LD HL, I     ; with its extension Iz
        0xE5,        // PUSH HL
        0xED, 0x3E,  // SWAP HL
        0xE5,        // PUSH HL
        0xED, 0x5F,  // LD A, R
        0x77,        // LD (HL), A   ; captured, reads back R
        0x18, 0x00,  // JR $+2
    };
    // clang-format on
    ASSERT_EVEN(SAVE_OTHERS);
    uint8_t others[5];
    _pins->captureWrites(
            SAVE_OTHERS, sizeof(SAVE_OTHERS), others, sizeof(others), org);
    _i = le32(others + 0, others + 2);
    _r = others[4];
    // Leave the CPU parked back where it was, so that between operations
    // "parked at parkedAt()" holds everywhere.
    uint8_t jp[7];
    const auto len = jumpTo(jp, parked);
    _pins->execInst(jp, len, org, parked);
    if (_vector == NONE)
        _parkedAddr = org;
    updateMode();
}

// The chip leaves SP, I and the bank 0 extensions as they were at power-up,
// though Table 7 has them cleared, and Z80 code with a stray HLz reaches
// far outside its 64K through (HL). Give them the documented values.
void RegsZ380::reset() {
    _sp = 0;
    _i = 0;
    for (uint_fast8_t p = 0; p < 2; ++p) {
        auto &b = _bank[0];
        b.bc[p] &= UINT16_MAX;
        b.de[p] &= UINT16_MAX;
        b.hl[p] &= UINT16_MAX;
        _ix[0][p] &= UINT16_MAX;
        _iy[0][p] &= UINT16_MAX;
    }
}

// One bank, both sides. F has no load instruction of its own, so both AF
// come back through real POPs of words staged just below the stack:
// injection answers only reads inside the window, and the stack is
// elsewhere.
void RegsZ380::restoreBank(uint_fast8_t bank, uint32_t &org) const {
    const auto &r = _bank[bank];
    const auto a = afp(_srCpu);  // the side the first POP AF lands in
    const uint32_t scratch = spPlus(_sp, -4);
    const uint8_t staged[] = {r.f[a], r.a[a], r.f[a ^ 1], r.a[a ^ 1]};
    for (uint_fast8_t i = 0; i < sizeof(staged); ++i)
        _mems->write_byte(spPlus(scratch, i) & MemsZ380::ADDR_MASK, staged[i]);
    const uint8_t dsr = bank << 1;
    // clang-format off
    const uint8_t RESTORE_BANK[] = {
        0xED, 0xDA, uint8_t(dsr | 1),  // LDCTL DSR, bank|ALT
        DDIR_IW_LW_0, DDIR_IW_LW_1, 0x01, lo8(r.bc[1]), hi8(r.bc[1]), lo8z(r.bc[1]), hi8z(r.bc[1]),
        DDIR_IW_LW_0, DDIR_IW_LW_1, 0x11, lo8(r.de[1]), hi8(r.de[1]), lo8z(r.de[1]), hi8z(r.de[1]),
        DDIR_IW_LW_0, DDIR_IW_LW_1, 0x21, lo8(r.hl[1]), hi8(r.hl[1]), lo8z(r.hl[1]), hi8z(r.hl[1]),
        0xED, 0xDA, dsr,               // LDCTL DSR, bank
        DDIR_IW_LW_0, DDIR_IW_LW_1, 0x01, lo8(r.bc[0]), hi8(r.bc[0]), lo8z(r.bc[0]), hi8z(r.bc[0]),
        DDIR_IW_LW_0, DDIR_IW_LW_1, 0x11, lo8(r.de[0]), hi8(r.de[0]), lo8z(r.de[0]), hi8z(r.de[0]),
        DDIR_IW_LW_0, DDIR_IW_LW_1, 0x21, lo8(r.hl[0]), hi8(r.hl[0]), lo8z(r.hl[0]), hi8z(r.hl[0]),
        DDIR_IW_LW_0, DDIR_IW_LW_1, 0x31, lo8(scratch), hi8(scratch), lo8z(scratch), hi8z(scratch),
        0xF1, 0x08, 0xF1, 0x08,        // POP AF; EX AF,AF'; POP AF; EX AF,AF'
        0x00,                          // NOP
        0x18, 0x00,                    // JR $+2: exit after the POPs
    };
    // clang-format on
    ASSERT_EVEN(RESTORE_BANK);
    _pins->execInst(RESTORE_BANK, sizeof(RESTORE_BANK), org);
}

void RegsZ380::restoreIndex(
        const uint32_t (&reg)[4][2], uint8_t prefix, uint32_t &org) const {
    uint8_t seq[88];
    uint_fast8_t len = 0;
    const auto put = [&](std::initializer_list<uint8_t> bytes) {
        for (auto b : bytes)
            seq[len++] = b;
    };
    for (uint8_t n = 0; n < 8; ++n) {
        const auto v = reg[n >> 1][n & 1];
        // LDCTL XSR/YSR, bank<<1|prime; DDIR IW,LW; LD IX/IY, v
        put({prefix, 0xDA, n, DDIR_IW_LW_0, DDIR_IW_LW_1, prefix, 0x21, lo8(v),
                hi8(v), lo8z(v), hi8z(v)});
    }
    _pins->execInst(seq, len, org);
}

void RegsZ380::restore() {
    // clang-format off
    const uint8_t LD_OTHERS[] = {
        0x3E, _r,                                      // LD A, _r
        0xED, 0x4F,                                    // LD R, A
        DDIR_IW_LW_0, DDIR_IW_LW_1,
        0x21, lo8(_i), hi8(_i), lo8z(_i), hi8z(_i),    // LD HL, _i
        0xFD, 0xC0, 0xDD, 0x47,                        // DDIR LW; LD I, HL
        0x00,                                          // NOP
    };
    // clang-format on
    ASSERT_EVEN(LD_OTHERS);
    auto org = parkedAt();
    _pins->execInst(LD_OTHERS, sizeof(LD_OTHERS), org);
    restoreIndex(_ix, 0xDD, org);
    restoreIndex(_iy, 0xFD, org);
    for (uint_fast8_t bank = 0; bank < 4; ++bank)
        restoreBank(bank, org);
    // Return the way the vector was taken: the pushed PC may have been
    // edited, so write it back first.
    auto sp = _sp;
    uint8_t ret[7];
    uint_fast8_t retLen = 0;
    if (_vector == NMI) {
        sp = spPlus(sp, -static_cast<int32_t>(pcSize()));
        for (uint_fast8_t i = 0; i < pcSize(); ++i)
            _mems->write_byte(
                    spPlus(sp, i) & MemsZ380::ADDR_MASK, _pc >> (i * 8));
        ret[retLen++] = InstZ380::RETN_PREFIX;
        ret[retLen++] = InstZ380::RETN;
    } else {
        retLen = jumpTo(ret, _pc);
    }
    // The selections last, as edited, through the byte forms of LDCTL so
    // that no bank's HL carries the SR: then Long Word mode, the one mode
    // bit the debugger changed, HL as selected, and SP. XM, IM and IEF1
    // it never touched; after an NMI, RETN restores IEF1 from IEF2.
    const auto hl = live().hl[alt(_sr)];
    uint8_t seq[40];
    uint_fast8_t len = 0;
    const auto put = [&](std::initializer_list<uint8_t> bytes) {
        for (auto b : bytes)
            seq[len++] = b;
    };
    // clang-format off
    put({0xED, 0xDA, uint8_t(_sr >> 8)});   // LDCTL DSR, dsr
    put({0xDD, 0xDA, uint8_t(_sr >> 16)});  // LDCTL XSR, xsr
    put({0xFD, 0xDA, uint8_t(_sr >> 24)});  // LDCTL YSR, ysr
    if (afp(_sr) != afp(_srCpu))
        put({0x08});                        // EX AF,AF'
    if ((_sr & SR_XM) && !(_srCpu & SR_XM))
        put({0xFD, 0xF7});                  // SETC XM
    if (_sr & SR_LW)
        put({0xDD, 0xF7});                  // SETC LW
    put({DDIR_IW_LW_0, DDIR_IW_LW_1, 0x21, lo8(hl), hi8(hl), lo8z(hl), hi8z(hl)});  // LD HL, hl
    put({DDIR_IW_LW_0, DDIR_IW_LW_1, 0x31, lo8(sp), hi8(sp), lo8z(sp), hi8z(sp)});  // LD SP, sp
    // clang-format on
    for (uint_fast8_t i = 0; i < retLen; ++i)
        put({ret[i]});
    _vector = NONE;
    _pins->execInst(seq, len, org, _pc);
    _parkedAddr = org;  // the fetch of the PC, as the bus showed it
}

void RegsZ380::helpRegisters() const {
    cli.println("?Reg: PC SP SR I R IX IY BC DE HL A F B C D E H L");
    cli.println("      EX EXX MB XB YB IXP IYP XM LW");
}

namespace {
constexpr const char *REGS8[] = {
        "F",  // 1
        "A",  // 2
        "B",  // 3
        "C",  // 4
        "D",  // 5
        "E",  // 6
        "H",  // 7
        "L",  // 8
        "R",  // 9
};
constexpr const char *REGS32[] = {
        "I",   // 10
        "PC",  // 11
        "SP",  // 12
        "SR",  // 13
        "BC",  // 14
        "DE",  // 15
        "HL",  // 16
        "IX",  // 17
        "IY",  // 18
};
constexpr const char *EXCHANGE[] = {
        "EX",   // 19
        "EXX",  // 20
};
constexpr const char *BANKS[] = {
        "MB",  // 21: MAINBANK
        "XB",  // 22: IXBANK
        "YB",  // 23: IYBANK
};
constexpr const char *SIDES[] = {
        "IXP",  // 26
        "IYP",  // 27
};
constexpr const char *MODES[] = {
        "XM",  // 91: Extended mode, which only reset clears
        "LW",  // 92: Long Word mode
};

void setHi(uint32_t &r, uint8_t v) {
    r = (r & ~UINT32_C(0xFF00)) | (static_cast<uint32_t>(v) << 8);
}
void setLo(uint32_t &r, uint8_t v) {
    r = (r & ~UINT32_C(0xFF)) | v;
}
void setBits(uint32_t &r, uint_fast8_t pos, uint32_t mask, uint32_t v) {
    r = (r & ~(mask << pos)) | ((v & mask) << pos);
}
}  // namespace

const Regs::RegList *RegsZ380::listRegisters(uint_fast8_t n) const {
    static constexpr RegList REG_LIST[] = {
            {REGS8, 9, 1, UINT8_MAX},
            {REGS32, 9, 10, UINT32_MAX},
            {EXCHANGE, 2, 19, 1},
            {BANKS, 3, 21, 3},
            {SIDES, 2, 26, 1},
            {MODES, 2, 91, 1},
    };
    return n < sizeof(REG_LIST) / sizeof(REG_LIST[0]) ? &REG_LIST[n] : nullptr;
}

bool RegsZ380::setRegister(uint_fast8_t reg, uint32_t value) {
    auto &bank = live();
    const auto a = afp(_sr), b = alt(_sr);
    switch (reg) {
    case 1:
        bank.f[a] = value;
        break;
    case 2:
        bank.a[a] = value;
        break;
    case 3:
        setHi(bank.bc[b], value);
        break;
    case 4:
        setLo(bank.bc[b], value);
        break;
    case 5:
        setHi(bank.de[b], value);
        break;
    case 6:
        setLo(bank.de[b], value);
        break;
    case 7:
        setHi(bank.hl[b], value);
        break;
    case 8:
        setLo(bank.hl[b], value);
        break;
    case 9:
        _r = value;
        break;
    case 10:
        _i = value;
        break;
    case 11:
        _pc = value;
        return true;
    case 12:
        _sp = value;
        break;
    case 13:
        // The mode bits stay the CPU's, but for Long Word mode.
        _sr = (value & ~SR_FIXED) | (_sr & SR_FIXED);
        updateMode();
        break;
    case 14:
        bank.bc[b] = value;
        break;
    case 15:
        bank.de[b] = value;
        break;
    case 16:
        bank.hl[b] = value;
        break;
    case 17:
        ix() = value;
        break;
    case 18:
        iy() = value;
        break;
    case 19:  // EX AF,AF' flips AFP
        _sr ^= 1;
        break;
    case 20:  // EXX flips ALT
        _sr ^= UINT32_C(1) << 8;
        break;
    case 21:
        setBits(_sr, 9, 3, value);
        break;
    case 22:
        setBits(_sr, 17, 3, value);
        break;
    case 23:
        setBits(_sr, 25, 3, value);
        break;
    case 26:
        setBits(_sr, 16, 1, value);
        break;
    case 27:
        setBits(_sr, 24, 1, value);
        break;
    case 91:  // SETC XM on resume; nothing leaves Extended mode but reset
        if (value)
            _sr |= SR_XM;
        updateMode();
        break;
    case 92:
        setBits(_sr, 6, 1, value);
        updateMode();
        break;
    }
    return false;
}

}  // namespace z380
}  // namespace debugger

// Local Variables:
// mode: c++
// c-basic-offset: 4
// tab-width: 4
// End:
// vim: set ft=cpp et ts=4 sw=4:
