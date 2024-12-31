#include "regs_mc6809.h"
#include "char_buffer.h"
#include "debugger.h"
#include "pins_mc6809.h"
#include "signals_mc6809.h"

namespace debugger {
namespace mc6809 {

/**
 * How to determine MC6809 variants.
 *
 * MC6809/HD6309
 *   CLRB
 *   FDB $1043
 *       ; NOP  on MC6809 ; 1:x:x
 *       ; COMD on HD6309 ; 1:2:N
 *   B=$00: MC6809
 *   B=$FF: HD6309
 */

SoftwareType RegsMc6809::checkSoftwareType() {
    static constexpr uint8_t DETECT_6309[] = {
            0x5F, 0x10,        // CLRB            ; 1:N
            0x10, 0x43, 0xF7,  // COMD  on HD6309 ; 1:2:N
                               // NOP   on MC6809 ; 1:x:x
            0xF7, 0x80, 0x00,  // STB  $8000      ; 1:2:3:B
    };
    _pins->injectReads(DETECT_6309, sizeof(DETECT_6309));
    uint8_t b;
    _pins->captureWrites(&b, sizeof(b));
    _type = (b == 0) ? SW_MC6809 : SW_HD6309;
    return _type;
}

const char *RegsMc6809::cpu() const {
    return _type == SW_MC6809 ? "MC6809" : "HD6309";
}

const char *RegsMc6809::cpuName() const {
    return cpu();
}

void RegsMc6809::print() const {
    static constexpr char line1[] =
            // 2345678901234567890123456789012345678901234567890123456789012
            "PC=xxxx S=xxxx X=xxxx Y=xxxx U=xxxx A=xx B=xx DP=xx CC=EFHINZVC";
    static auto &buffer1 = *new CharBuffer(line1);
    buffer1.hex16(3, _pc);
    buffer1.hex16(10, _s);
    buffer1.hex16(17, _x);
    buffer1.hex16(24, _y);
    buffer1.hex16(31, _u);
    buffer1.hex8(38, _a);
    buffer1.hex8(43, _b);
    buffer1.hex8(49, _dp);
    buffer1.bits(55, _cc, 0x80, line1 + 55);
    cli.println(buffer1);
    _pins->idle();
    if (_type == SW_MC6809)
        return;

    static constexpr char line2[] =
            // 234567890123456789012345678901234567890123456789
            "               D=xxxx W=xxxx V=xxxx E=xx F=xx MD=x";
    static auto &buffer2 = *new CharBuffer(line2);
    buffer2.hex16(17, _d());
    buffer2.hex16(24, _w());
    buffer2.hex16(31, _v);
    buffer2.hex8(38, _e);
    buffer2.hex8(43, _f);
    buffer2.hex1(49, _md);
    cli.println(buffer2);
    _pins->idle();
}

void RegsMc6809::reset() {
    loadStack(0x8000);  // pre-decrement stack
}

void RegsMc6809::save() {
    uint8_t context[14];
    uint16_t sp;
    const auto n = _pins->captureContext(context, sp);
    saveContext(context, n, sp + 1);
    // Capturing writes to stack in little endian order.
    _pc -= 1;  //  offset SWI instruction.
}

void RegsMc6809::restore() {
    if (_type == SW_HD6309) {
        loadMode(_md);
        loadVW();
    }
    loadStack(_s - (_md ? 14 : 12));
    // 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6
    // 1:N:R:r:r:r:r:r:r:r:r:r:r:r:X     MC6809
    // 1:N:R:r:r:r:r:r:r:r:r:r:r:r:r:r:X HD6309 native
    uint8_t RTI[16];
    RTI[0] = 0x3B;  // RTI
    auto cycle = 2;
    RTI[cycle++] = _cc;
    RTI[cycle++] = _a;
    RTI[cycle++] = _b;
    if (_md) {
        RTI[cycle++] = _e;
        RTI[cycle++] = _f;
    }
    RTI[cycle++] = _dp;
    be16(&RTI[cycle + 0], _x);
    be16(&RTI[cycle + 2], _y);
    be16(&RTI[cycle + 4], _u);
    be16(&RTI[cycle + 6], _pc);
    _pins->injectReads(RTI, cycle + 8, cycle + 9);
}

uint8_t RegsMc6809::contextLength() const {
    return _md ? 14 : 12;
}

uint16_t RegsMc6809::capture(const Signals *frame, bool step) {
    const auto sp = frame->addr;
    uint8_t context[14];
    uint8_t cap = 0;
    for (auto s = frame; s->write(); s = s->next())
        context[cap++] = s->data;
    saveContext(context, cap, sp + 1U);
    if (!step)
        _pc -= 1;  // offset SWI
    return _pc;
}

void RegsMc6809::saveContext(uint8_t *context, uint8_t n, uint16_t sp) {
    _s = sp;
    _md = (n == 14);
    // Capturing writes to stack in little endian order.
    _pc = le16(context + 0);
    _u = le16(context + 2);
    _y = le16(context + 4);
    _x = le16(context + 6);
    _dp = context[8];
    uint8_t i = 8;
    if (_md) {
        _f = context[++i];
        _e = context[++i];
    }
    _b = context[++i];
    _a = context[++i];
    _cc = context[++i];
    if (_type == SW_HD6309)
        saveVW();
}

void RegsMc6809::loadStack(uint16_t sp) const {
    const uint8_t LDS[4] = {
            0x10, 0xCE, hi(sp), lo(sp),  // LDS #sp
    };
    _pins->injectReads(LDS, sizeof(LDS));
}

void RegsMc6809::loadMode(bool native) const {
    uint8_t LDMD[3] = {
            0x11, 0x3D, uint8(native ? 1 : 0),  // LDMD #native
    };
    _pins->injectReads(LDMD, sizeof(LDMD), 5);
}

uint16_t RegsMc6809::saveW() const {
    static constexpr uint8_t STW[] = {
            0x10, 0xB7,  // STW $8000 ; 1:2:3:4:x:B:b (HD6309)
            0x80, 0x00,  //           ; 1:2:3:4:B:b   (HD6309 native)
    };
    _pins->injectReads(STW, sizeof(STW));
    uint8_t buffer[2];
    _pins->captureWrites(buffer, sizeof(buffer));
    return be16(buffer);
}

void RegsMc6809::saveVW() {
    _w(saveW());
    static constexpr uint8_t TFR[] = {
            0x1F, 0x76,  // TFR V,W ; 1:2:x:x:x:x (HD6309)
                         //         ; 1:2:x:x     (HD6309 native)
    };
    _pins->injectReads(TFR, sizeof(TFR), _md ? 4 : 6);
    _v = saveW();
}

void RegsMc6809::loadW(uint16_t val) const {
    const uint8_t LDW[4] = {
            0x10, 0x86, hi(val), lo(val),  // LDW #val ; 1:2:3:4
    };
    _pins->injectReads(LDW, sizeof(LDW));
}

void RegsMc6809::loadVW() const {
    loadW(_v);
    static constexpr uint8_t TFR[] = {
            0x1F, 0x67,  // TFR W,V ; 1:2:x:x:x:x (HD6309)
                         //         ; 1:2:x:x     (HD6309 native)
    };
    _pins->injectReads(TFR, sizeof(TFR), _md ? 4 : 6);
    loadW(_w());
}

void RegsMc6809::helpRegisters() const {
    cli.print("?Reg: PC S U Y X A B D");
    if (_type == SW_HD6309)
        cli.print(" E F W Q V");
    cli.println(" CC DP");
}

constexpr const char *REGS8[] = {
        "A",   // 1
        "B",   // 2
        "CC",  // 3
        "DP",  // 4
};
constexpr const char *REGS16[] = {
        "PC",  // 5
        "SP",  // 6
        "S",   // 7
        "U",   // 8
        "X",   // 9
        "Y",   // 10
        "D",   // 11
};
constexpr const char *REGS8_6309[] = {
        "E",  // 12
        "F",  // 13
};
constexpr const char *REGS16_6309[] = {
        "W",  // 14
        "V",  // 15
};
constexpr const char *REGS32_6309[] = {
        "Q",  // 16
};
constexpr const char *REGS1_6309[] = {
        "MD",  // 17
};

const Regs::RegList *RegsMc6809::listRegisters(uint8_t n) const {
    static constexpr RegList REG_LIST[] = {
            {REGS8, 4, 1, UINT8_MAX},
            {REGS16, 7, 5, UINT16_MAX},
            {REGS8_6309, 2, 12, UINT8_MAX},
            {REGS16_6309, 2, 14, UINT16_MAX},
            {REGS32_6309, 1, 16, UINT32_MAX},
            {REGS1_6309, 1, 17, 1},
    };
    const auto r = (_type == SW_MC6809) ? 2 : 6;
    return n < r ? &REG_LIST[n] : nullptr;
}

void RegsMc6809::setRegister(uint8_t reg, uint32_t value) {
    switch (reg) {
    case 1:
        _a = value;
        break;
    case 2:
        _b = value;
        break;
    case 3:
        _cc = value;
        break;
    case 4:
        _dp = value;
        break;
    case 5:
        _pc = value;
        break;
    case 6:
    case 7:
        _s = value;
        break;
    case 8:
        _u = value;
        break;
    case 9:
        _x = value;
        break;
    case 10:
        _y = value;
        break;
    case 11:
        _d(value);
        break;
    case 12:
        _e = value;
        break;
    case 13:
        _f = value;
        break;
    case 14:
        _w(value);
        break;
    case 15:
        _v = value;
        break;
    case 16:
        _q(value);
        break;
    case 17:
        _md = value;
        break;
    }
}

}  // namespace mc6809
}  // namespace debugger

// Local Variables:
// mode: c++
// c-basic-offset: 4
// tab-width: 4
// End:
// vim: set ft=cpp et ts=4 sw=4:
