#include "regs_mc6809.h"
#include "char_buffer.h"
#include "debugger.h"
#include "digital_bus.h"
#include "mems_mc6809.h"
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
    // clang-format off
    //                               012345678901234567890123456789012345678901234567890123456789012
    static constexpr char line1[] = "DP=xx PC=xxxx S=xxxx U=xxxx Y=xxxx X=xxxx A=xx B=xx CC=EFHINZVC";
    // clang-format on
    static auto &buffer1 = *new CharBuffer(line1);
    buffer1.hex8(3, _dp);
    buffer1.hex16(9, _pc);
    buffer1.hex16(16, _s);
    buffer1.hex16(23, _u);
    buffer1.hex16(30, _y);
    buffer1.hex16(37, _x);
    buffer1.hex8(44, _a);
    buffer1.hex8(49, _b);
    buffer1.bits(55, _cc, 0x80, line1 + 55);
    cli.print(buffer1);
    if (_type == SW_HD6309) {
        //                               0123456789012345
        static constexpr char line2[] = " W=xxxx V=xxxx N";
        static auto &buffer2 = *new CharBuffer(line2);
        buffer2.hex8(3, _e);
        buffer2.hex8(5, _f);
        buffer2.hex16(10, _v);
        buffer2[14] = _native6309 ? ' ' : 0;
        cli.print(buffer2);
    }
    cli.println();
    _pins->idle();
}

void RegsMc6809::reset() const {
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
        loadMode(_native6309);
        loadVW();
    }
    loadStack(_s - (_native6309 ? 14 : 12));
    // 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6
    // 1:N:R:r:r:r:r:r:r:r:r:r:r:r:X     MC6809
    // 1:N:R:r:r:r:r:r:r:r:r:r:r:r:r:r:X HD6309 native
    uint8_t RTI[16];
    RTI[0] = 0x3B;  // RTI
    auto cycle = 2;
    RTI[cycle++] = _cc;
    RTI[cycle++] = _a;
    RTI[cycle++] = _b;
    if (_native6309) {
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
    return _native6309 ? 14 : 12;
}

uint16_t RegsMc6809::capture(const Signals *frame, bool step) {
    const uint16_t sp = frame->addr;
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
    _native6309 = (n == 14);
    // Capturing writes to stack in little endian order.
    _pc = le16(context + 0);
    _u = le16(context + 2);
    _y = le16(context + 4);
    _x = le16(context + 6);
    _dp = context[8];
    uint8_t i = 8;
    if (_native6309) {
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
    uint8_t LDS[4] = {
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

void RegsMc6809::saveVW() {
    static constexpr uint8_t STW[] = {
            0x10, 0xB7,  // STW $8000 ; 1:2:3:4:x:B:b (HD6309)
            0x80, 0x00,  //           ; 1:2:3:4:B:b   (HD6309 native)
    };
    _pins->injectReads(STW, sizeof(STW));
    uint8_t buffer[2];
    _pins->captureWrites(buffer, sizeof(buffer));
    _e = buffer[0];
    _f = buffer[1];
    static constexpr uint8_t TFR[] = {
            0x1F, 0x76,  // TFR V,W ; 1:2:x:x:x:x (HD6309)
                         //         ; 1:2:x:x     (HD6309 native)
    };
    _pins->injectReads(TFR, sizeof(TFR), _native6309 ? 4 : 6);
    _pins->injectReads(STW, sizeof(STW));
    _pins->captureWrites(buffer, sizeof(buffer));
    _v = be16(buffer);
}

void RegsMc6809::loadVW() const {
    uint8_t LDW[4] = {
            0x10, 0x86, hi(_v), lo(_v),  // LDW #_v ; 1:2:3:4
    };
    _pins->injectReads(LDW, sizeof(LDW));
    static constexpr uint8_t TFR[] = {
            0x1F, 0x67,  // TFR W,V ; 1:2:x:x:x:x (HD6309)
                         //         ; 1:2:x:x     (HD6309 native)
    };
    _pins->injectReads(TFR, sizeof(TFR), _native6309 ? 4 : 6);
    LDW[2] = _e;
    LDW[3] = _f;
    _pins->injectReads(LDW, sizeof(LDW));
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

const Regs::RegList *RegsMc6809::listRegisters(uint8_t n) const {
    static constexpr RegList REG_LIST[] = {
            {REGS8, 4, 1, UINT8_MAX},
            {REGS16, 7, 5, UINT16_MAX},
            {REGS8_6309, 2, 12, UINT8_MAX},
            {REGS16_6309, 2, 14, UINT16_MAX},
            {REGS32_6309, 1, 16, UINT32_MAX},
    };
    const auto r = (_type == SW_MC6809) ? 2 : 5;
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
