#include "regs_tlcs90.h"
#include "char_buffer.h"
#include "debugger.h"
#include "digital_bus.h"
#include "mems_tlcs90.h"
#include "pins_tlcs90.h"

namespace debugger {
namespace tlcs90 {

struct RegsTlcs90 Regs;

const char *RegsTlcs90::cpu() const {
    return "TLC90";
}

const char *RegsTlcs90::cpuName() const {
    return "TMP90C802";
}

void RegsTlcs90::print() const {
    // clang-format off
    //                              01234567890123456789012345678901234567890123456789012345
    static constexpr char main[] = "PC=xxxx SP=xxxx  BC=xxxx DE=xxxx HL=xxxx A=xx F=SZIHXVNC";
    static constexpr char alt[]  = "IX=xxxx IY=xxxx (BC=xxxx DE=xxxx HL=xxxx A=xx F=SZIHXVNC)";
    // clang-format on
    static auto &bufmain = *new CharBuffer(main);
    static auto &bufalt = *new CharBuffer(alt);
    bufmain.hex16(3, _pc);
    bufmain.hex16(11, _sp);
    bufmain.hex16(20, _main.bc());
    bufmain.hex16(28, _main.de());
    bufmain.hex16(36, _main.hl());
    bufmain.hex8(43, _main.a);
    bufmain.bits(48, _main.f, 0x80, &main[48]);
    cli.println(bufmain);
    Pins.idle();
    bufalt.hex16(3, _ix);
    bufalt.hex16(11, _iy);
    bufalt.hex16(20, _alt.bc());
    bufalt.hex16(28, _alt.de());
    bufalt.hex16(36, _alt.hl());
    bufalt.hex8(43, _alt.a);
    bufalt.bits(48, _alt.f, 0x80, &alt[48]);
    cli.println(bufalt);
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
    Pins.execInst(CONFIG, sizeof(CONFIG));
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
            0x56,                    // PUSH AF;      ; 0:N:d:W:W
            0x00,                    // NOP           ; 0:N
    };
    uint8_t buffer[4];
    Pins.captureWrites(SAVE_SP, sizeof(SAVE_SP), buffer, sizeof(buffer), &_pc);
    _sp = le16(buffer + 0);
    _main.a = buffer[2];
    _main.f = buffer[3];
    saveRegisters();
}

void RegsTlcs90::saveRegisters() {
    static constexpr uint8_t SAVE_ALL[] = {
            0x09,  // EX AF,AF' ; 0:N
            0x56,  // PUSH AF   ; 0:N:d:W:W
            0x09,  // EX AF,AF' ; 0:N
            0x50,  // PUSH BC   ; 0:N:d:W:W
            0x51,  // PUSH DE   ; 0:N:d:W:W
            0x52,  // PUSH HL   ; 0:N:d:W:W
            0x54,  // PUSH IX   ; 0:N:d:W:W
            0x55,  // PUSH IY   ; 0:N:d:W:W
            0x0A,  // EXX       ; 0:N
            0x50,  // PUSH BC   ; 0:N:d:W:W
            0x51,  // PUSH DE   ; 0:N:d:W:W
            0x52,  // PUSH HL   ; 0:N:d:W:W
            0x0A,  // EXX       ; 0:N
    };
    uint8_t buffer[2 * 9];
    Pins.captureWrites(SAVE_ALL, sizeof(SAVE_ALL), buffer, sizeof(buffer));
    _alt.a = buffer[0];
    _alt.f = buffer[1];
    _main.b = buffer[2];
    _main.c = buffer[3];
    _main.d = buffer[4];
    _main.e = buffer[5];
    _main.h = buffer[6];
    _main.l = buffer[7];
    _ix = be16(buffer + 8);
    _iy = be16(buffer + 10);
    _alt.b = buffer[12];
    _alt.c = buffer[13];
    _alt.d = buffer[14];
    _alt.e = buffer[15];
    _alt.h = buffer[16];
    _alt.l = buffer[17];
}

void RegsTlcs90::restore() {
    uint8_t LD_ALL[42] = {
            0x09,              //  0: EX AF,AF' ; 0:N
            0x5E, 0x09, 0, 0,  //  1: POP AF    ; 0:n:R:R:d:N
            0x09,              //  5: EX AF,AF' ; 0:N
            0x5E, 0x09, 0, 0,  //  6: POP AF    ; 0:n:R:R:d:N
            0x0A,              // 10: EXX       ; 0:N
            0x38, 0, 0,        // 11: LD BC,mn  ; 0:2:3:N
            0x39, 0, 0,        // 14: LD DE,mn  ; 0:2:3:N
            0x3A, 0, 0,        // 17: LD HL,nm  ; 0:2:3:N
            0x0A,              // 20: EXX       ; N
            0x38, 0, 0,        // 21: LD BC,mn  ; 0:2:3:N
            0x39, 0, 0,        // 24: LD DE,mn  ; 0:2:3:N
            0x3A, 0, 0,        // 27: LD HL,nm  ; 0:2:3:N
            0x3C, 0, 0,        // 30: LD IX,nm  ; 0:2:3:N
            0x3D, 0, 0,        // 33: LD IY,nm  ; 0:2:3:N
            0x3E, 0, 0,        // 36: LD SP,nm  ; 0:2:3:N
            0x1A, 0, 0,        // 39: JP nm     ; 0:2:3:d:J
    };
    LD_ALL[3] = _alt.f;
    LD_ALL[4] = _alt.a;
    LD_ALL[8] = _main.f;
    LD_ALL[9] = _main.a;
    LD_ALL[12] = _alt.c;
    LD_ALL[13] = _alt.b;
    LD_ALL[15] = _alt.e;
    LD_ALL[16] = _alt.d;
    LD_ALL[18] = _alt.l;
    LD_ALL[19] = _alt.h;
    LD_ALL[22] = _main.c;
    LD_ALL[23] = _main.b;
    LD_ALL[25] = _main.e;
    LD_ALL[26] = _main.d;
    LD_ALL[28] = _main.l;
    LD_ALL[29] = _main.h;
    setle16(LD_ALL + 31, _ix);
    setle16(LD_ALL + 34, _iy);
    setle16(LD_ALL + 37, _sp);
    setle16(LD_ALL + 40, _pc);
    Pins.execInst(LD_ALL, sizeof(LD_ALL));
}

void RegsTlcs90::helpRegisters() const {
    cli.println(F("?Reg: PC SP IX IY BC DE HL A F B C D E H L EX EXX"));
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

uint8_t RegsTlcs90::internal_read(uint16_t addr) {
    static uint8_t LD[] = {
            0xE3, 0, 0, 0x2E,  // LD A,(mn) ; 0:2:3:7:R:N
            0xEB, 0, 0, 0x26,  // LD (mn),A ; 0:2:3:7:N:W
            0x00,              // NOP       ; 0:N
    };
    LD[1] = lo(addr);
    LD[2] = hi(addr);
    Pins.captureWrites(LD, sizeof(LD), &LD[2], 1);
    return LD[2];
}

void RegsTlcs90::internal_write(uint16_t addr, uint8_t data) {
    static uint8_t LD[] = {
            0x36, 0,           // LD A,n    ; 0:2:N
            0xEB, 0, 0, 0x26,  // LD (mn),A ; 0:2:3:7:N:W
            0x00,              // NOP       ; 0:N
    };
    LD[1] = data;
    LD[3] = lo(addr);
    LD[4] = hi(addr);
    Pins.execInst(LD, sizeof(LD));
}

}  // namespace tlcs90
}  // namespace debugger

// Local Variables:
// mode: c++
// c-basic-offset: 4
// tab-width: 4
// End:
// vim: set ft=cpp et ts=4 sw=4:
