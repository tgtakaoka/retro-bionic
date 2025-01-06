#include "regs_mc68hc11.h"
#include "debugger.h"
#include "mc68hc11_init.h"
#include "pins_mc68hc11.h"

namespace debugger {
namespace mc68hc11 {
namespace {
// clang-format off
//                   012345678901234567890123456789012345678901234567890
const char line[] = "PC=xxxx SP=xxxx X=xxxx Y=xxxx A=xx B=xx CC=SXHINZVC";
// clang-format on
}  // namespace

RegsMc68hc11::RegsMc68hc11(mc6800::PinsMc6800Base *pins, Mc68hc11Init &init)
    : RegsMc6800(pins), _init(init) {
    _buffer.set(line);
}

/**
 * How to determine MC6800 variants.
 *
 * MC6800/MC6801/HD6301
 *   LDX  #$55AA
 *   LDAB #100
 *   LDAA #5
 *   ADDA #5
 *   FCB  $18
 *        ; DAA  ($19) on MC6800
 *        ; ABA  ($1B) on MC6801
 *        ; XGDX ($18) on HD6301
 *        ; prefix     on MC68HC11
 *   FCB  $08
 *        ; INX on MC6800/MC6801/HD6301
 *        ; INY on MC68HC11
 *   A=$10, X=$55AB: MC6800
 *   A=110, X=$55AB: MC6801
 *   A=$55, X=$0A65: HD6301
 *   A=10,  X=$55AA: MC68HC11
 *
 * MC6800/MB8861(MB8870)
 *   LDX  #$FFFF
 *   FCB  $EC, $01
 *        ; CPX 1,X ($AC $01, 6 clcoks) on MC6800
 *        ; ADX #1  ($EC $01, 2 clocks) on MB8861
 * X=$FFFF: MC6800
 * X=$0000: MB8861
 */

SoftwareType RegsMc68hc11::checkSoftwareType() {
    return _type = SW_MC68HC11;
}

const char *RegsMc68hc11::cpuName() const {
    return Debugger.target().identity();
}

void RegsMc68hc11::print() const {
    _buffer.hex16(3, _pc);
    _buffer.hex16(11, _sp);
    _buffer.hex16(18, _x);
    _buffer.hex16(25, _y);
    _buffer.hex8(32, _a);
    _buffer.hex8(37, _b);
    _buffer.bits(43, _cc, 0x80, line + 43);
    cli.println(_buffer);
}

void RegsMc68hc11::save() {
    static const uint8_t SWI = 0x3F;  // 1:N:w:W:W:W:W:W:W:W:W:n:V:v
    _pins->injectReads(&SWI, sizeof(SWI));
    uint8_t context[9];
    _pins->captureWrites(context, sizeof(context), &_sp);
    // Capturing writes to stack in little endian order.
    _pc = le16(context) - 1;  //  offset SWI instruction.
    _y = le16(context + 2);
    _x = le16(context + 4);
    _a = context[6];
    _b = context[7];
    _cc = context[8];
    // Read SWI vector
    context[0] = 0;  // irrelevant data
    context[1] = hi(_pc);
    context[2] = lo(_pc);
    _pins->injectReads(context, 3);
}

void RegsMc68hc11::restore() {
    _cc &= ~0x40;  // clear X bit to enable #XIRQ for step/suspend
    const uint16_t sp = _sp - 9;
    // clang-format off
    const uint8_t LDS_RTI[3 + 12] = {
        0x8E, hi(sp), lo(sp),   // LDS #sp ; 1:2:3
        0x3B, 0, 0,             // RTI     ; 1:N:n:R:r:r:r:r:r:r:r:r
        _cc, _b, _a,
        hi(_x), lo(_x),
        hi(_y), lo(_y),
        hi(_pc), lo(_pc),
    };
    // clang-format on
    _pins->injectReads(LDS_RTI, sizeof(LDS_RTI));
}

uint16_t RegsMc68hc11::capture(const mc6800::Signals *stack, bool step) {
    _sp = stack->addr;
    _pc = uint16(stack->next(1)->data, stack->data);
    if (!step)
        --_pc;
    _y = uint16(stack->next(3)->data, stack->next(2)->data);
    _x = uint16(stack->next(5)->data, stack->next(4)->data);
    _a = stack->next(6)->data;
    _b = stack->next(7)->data;
    _cc = stack->next(8)->data;
    return _pc;
}

void RegsMc68hc11::helpRegisters() const {
    cli.println("?Reg: PC SP X Y A B D CC");
}

constexpr const char *REGS8[] = {
        "A",   // 1
        "B",   // 2
        "CC",  // 3
};
constexpr const char *REGS16[] = {
        "PC",  // 4
        "SP",  // 5
        "X",   // 6
        "Y",   // 7
        "D",   // 8
};

const Regs::RegList *RegsMc68hc11::listRegisters(uint8_t n) const {
    static constexpr RegList REG_LIST[] = {
            {REGS8, 3, 1, UINT8_MAX},
            {REGS16, 3, 5, UINT16_MAX},
    };
    return n < 2 ? &REG_LIST[n] : nullptr;
}

void RegsMc68hc11::setRegister(uint8_t reg, uint32_t value) {
    switch (reg) {
    case 4:
        _pc = value;
        break;
    case 5:
        _sp = value;
        break;
    case 6:
        _x = value;
        break;
    case 1:
        _a = value;
        break;
    case 2:
        _b = value;
        break;
    case 3:
        _cc = value;
        break;
    case 7:
        _y = value;
        break;
    case 8:
        _d(value);
        break;
    }
}

uint8_t RegsMc68hc11::internal_read(uint16_t addr) const {
    uint8_t LDAA[] = {
            0xB6, hi(addr), lo(addr),  // LDAA addr ; 1:2:3:A
    };
    _pins->injectReads(LDAA, sizeof(LDAA), 4);
    static constexpr uint8_t STAA_FF00[] = {0xB7, 0xFF, 0x00};
    const auto staa = _init.is_internal(0x8000) ? STAA_FF00 : STAA_8000;
    _pins->injectReads(staa, sizeof(STAA_8000));
    uint8_t data;
    _pins->captureWrites(&data, sizeof(data));
    return data;
}

void RegsMc68hc11::internal_write(uint16_t addr, uint8_t data) const {
    uint8_t LDAA_STAA[] = {
            0x86, data,                // LDAA #val ; 1:2
            0xB7, hi(addr), lo(addr),  // STAA addr ; 1:2:3:B
    };
    _pins->injectReads(LDAA_STAA, sizeof(LDAA_STAA), 6);
}

}  // namespace mc68hc11
}  // namespace debugger

// Local Variables:
// mode: c++
// c-basic-offset: 4
// tab-width: 4
// End:
// vim: set ft=cpp et ts=4 sw=4:
