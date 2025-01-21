#include "regs_tlcs90.h"
#include "debugger.h"
#include "pins_tlcs90.h"

namespace debugger {
namespace tlcs90 {
namespace {
// clang-format off
//                              1         2         3         4         5
//                    01234567890123456789012345678901234567890123456789012345
const char line1[] = "PC=xxxx SP=xxxx  BC=xxxx DE=xxxx HL=xxxx A=xx F=SZIHXVNC";
const char line2[] = "IX=xxxx IY=xxxx (BC=xxxx DE=xxxx HL=xxxx A=xx F=SZIHXVNC)";
// clang-format on
}  // namespace

RegsTlcs90::RegsTlcs90(PinsTlcs90 *pins)
    : _pins(pins), _buffer1(line1), _buffer2(line2) {}

const char *RegsTlcs90::cpu() const {
    return "TLC90";
}

void RegsTlcs90::print() const {
    auto &main = _buffer1;
    main.hex16(3, _pc);
    main.hex16(11, _sp);
    main.hex16(20, _main.bc());
    main.hex16(28, _main.de());
    main.hex16(36, _main.hl());
    main.hex8(43, _main.a);
    main.bits(48, _main.f, 0x80, main + 48);
    cli.println(main);
    auto &alt = _buffer2;
    alt.hex16(3, _ix);
    alt.hex16(11, _iy);
    alt.hex16(20, _alt.bc());
    alt.hex16(28, _alt.de());
    alt.hex16(36, _alt.hl());
    alt.hex8(43, _alt.a);
    alt.bits(48, _alt.f, 0x80, alt + 48);
    cli.println(alt);
}

void RegsTlcs90::reset() {
    static constexpr uint8_t CONFIG[] = {
            /* Set normal-wait at P3CR */
            0x37, 0xC7, 0x40,  // LD (P3CR), 40H   ; 0:2:d:3:F:N
            /* Disable watchdog timer */
            0x37, 0xD2, 0x00,  // LD (WDMOD), ~WDTE; 0:2:d:3:F:N
            0x37, 0xD3, 0xB1,  // LD (WDCR), 0B1H  ; 0:2:d:2:F:N
            /* Jump to reset */
            0x3E, 0x00, 0x10,  // LD SP,1000H      ; 0:2:3:N
            0x1A, 0x00, 0x00,  // JP 0000H         ; 0:2:3:d:J
    };
    _pins->execInst(CONFIG, sizeof(CONFIG));
}

bool RegsTlcs90::saveContext(const Signals *frame) {
    // Machine context were pushed in the following order; PCH, PCL, A, F
    const auto pch = frame;
    const auto pcl = frame->next();
    if (pch->write() && pcl->write()) {
        _pc = uint16(pch->data, pcl->data);
        _sp = pch->addr + 1;
        const auto a = frame->next(2);
        const auto f = frame->next(3);
        if (a->write() && f->write()) {
            _main.a = a->data;
            _main.f = f->data;
            return true;
        }
    }
    return false;
}

void RegsTlcs90::save() {
    static constexpr uint8_t SAVE_SP[] = {
            0xEB, 0x00, 0x00, 0x46,  // LD (0000H),SP ; 0:2:3:7:N:W:W
            0x00,                    // NOP           ; 0:N
    };
    uint8_t buffer[2];
    _pins->captureWrites(
            SAVE_SP, sizeof(SAVE_SP), buffer, sizeof(buffer), &_pc);
    _sp = le16(buffer);
    saveRegisters();
}

void RegsTlcs90::saveRegisters() {
    saveRegs(_main);
    exchangeRegs();
    saveRegs(_alt);
    exchangeRegs();
    static constexpr uint8_t SAVE_INDEX[] = {
            0x54,  // PUSH IX   ; 0:N:d:W:W
            0x55,  // PUSH IY   ; 0:N:d:W:W
            0x00,  // NOP       ; 0:N
    };
    uint8_t buffer[4];
    _pins->captureWrites(
            SAVE_INDEX, sizeof(SAVE_INDEX), buffer, sizeof(buffer));
    _ix = be16(buffer + 0);
    _iy = be16(buffer + 2);
}

void RegsTlcs90::exchangeRegs() const {
    static constexpr uint8_t EXCHANGE[] = {
            0x09,  // EX AF,AF' ; 0:N
            0x0A,  // EXX       ; 0:N
    };
    _pins->execInst(EXCHANGE, sizeof(EXCHANGE));
}

void RegsTlcs90::saveRegs(reg &regs) const {
    static constexpr uint8_t SAVE_ALL[] = {
            0x56,  // PUSH AF   ; 0:N:d:W:W
            0x50,  // PUSH BC   ; 0:N:d:W:W
            0x51,  // PUSH DE   ; 0:N:d:W:W
            0x52,  // PUSH HL   ; 0:N:d:W:W
            0x00,  // NOP       ; 0:N
    };
    _pins->captureWrites(
            SAVE_ALL, sizeof(SAVE_ALL), (uint8_t *)&regs, sizeof(regs));
}

void RegsTlcs90::restore() {
    restoreRegs(_main);
    exchangeRegs();
    restoreRegs(_alt);
    exchangeRegs();
    const uint8_t LOAD_ALL[] = {
            0x3C, lo(_ix), hi(_ix),  // LD IX, _ix  ; 0:2:3:N
            0x3D, lo(_iy), hi(_iy),  // LD IY, _iy  ; 0:2:3:N
            0x3E, lo(_sp), hi(_sp),  // LD SP, _sp  ; 0:2:3:N
            0x1A, lo(_pc), hi(_pc),  // JP _pc     ; 0:2:3:d:J
    };
    _pins->execInst(LOAD_ALL, sizeof(LOAD_ALL));
}

void RegsTlcs90::restoreRegs(const reg &regs) const {
    const uint8_t LOAD_ALL[] = {
            0x5E, 0x09, regs.f, regs.a,  // POP AF    ; 0:n:R:R:d:N
            0x38, regs.c, regs.b,        // LD BC, _bc  ; 0:2:3:N
            0x39, regs.e, regs.d,        // LD DE, _de  ; 0:2:3:N
            0x3A, regs.l, regs.h,        // LD HL, _hl  ; 0:2:3:N
    };
    _pins->execInst(LOAD_ALL, sizeof(LOAD_ALL));
}

void RegsTlcs90::helpRegisters() const {
    cli.println("?Reg: PC SP IX IY BC DE HL A F B C D E H L EX EXX");
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
};
constexpr const char *REGS16[] = {
        "PC",  // 9
        "SP",  // 10
        "IX",  // 11
        "IY",  // 12
        "BC",  // 13
        "DE",  // 14
        "HL",  // 15
};
constexpr const char *EXCHANGE[] = {
        "EX",   // 16
        "EXX",  // 17
};

const Regs::RegList *RegsTlcs90::listRegisters(uint8_t n) const {
    static constexpr RegList REG_LIST[] = {
            {REGS8, 8, 1, UINT8_MAX},
            {REGS16, 7, 9, UINT16_MAX},
            {EXCHANGE, 2, 16, 1},
    };
    return n < 3 ? &REG_LIST[n] : nullptr;
}

void RegsTlcs90::setRegister(uint8_t reg, uint32_t value) {
    switch (reg) {
    case 9:
        _pc = value;
        break;
    case 10:
        _sp = value;
        break;
    case 11:
        _ix = value;
        break;
    case 12:
        _iy = value;
        break;
    case 13:
        _main.setbc(value);
        break;
    case 14:
        _main.setde(value);
        break;
    case 15:
        _main.sethl(value);
        break;
    case 2:
        _main.a = value;
        break;
    case 1:
        _main.f = value;
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
    case 16:
        swap8(_main.a, _alt.a);
        swap8(_main.f, _alt.f);
        break;
    case 17:
        swap8(_main.b, _alt.b);
        swap8(_main.c, _alt.c);
        swap8(_main.d, _alt.d);
        swap8(_main.e, _alt.e);
        swap8(_main.h, _alt.h);
        swap8(_main.l, _alt.l);
        break;
    }
}

uint8_t RegsTlcs90::internal_read(uint16_t addr) const {
    static uint8_t LD[] = {
            0xE3, 0, 0, 0x2E,  // LD A,(mn) ; 0:2:3:7:R:N
            0xEB, 0, 0, 0x26,  // LD (mn),A ; 0:2:3:7:N:W
            0x00,              // NOP       ; 0:N
    };
    LD[1] = lo(addr);
    LD[2] = hi(addr);
    _pins->captureWrites(LD, sizeof(LD), &LD[2], 1);
    return LD[2];
}

void RegsTlcs90::internal_write(uint16_t addr, uint8_t data) const {
    static uint8_t LD[] = {
            0x36, 0,           // LD A,n    ; 0:2:N
            0xEB, 0, 0, 0x26,  // LD (mn),A ; 0:2:3:7:N:W
            0x00,              // NOP       ; 0:N
    };
    LD[1] = data;
    LD[3] = lo(addr);
    LD[4] = hi(addr);
    _pins->execInst(LD, sizeof(LD));
}

}  // namespace tlcs90
}  // namespace debugger

// Local Variables:
// mode: c++
// c-basic-offset: 4
// tab-width: 4
// End:
// vim: set ft=cpp et ts=4 sw=4:
