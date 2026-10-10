#include "regs_mc68hc12.h"
#include "debugger.h"
#include "inst_mc68hc12.h"
#include "mc68hc12_init.h"
#include "pins_mc68hc12.h"

namespace debugger {
namespace mc68hc12 {

namespace {
//                    0123456789012345678901234567890123456789012345678
constexpr char line[] = "PC=xxxx SP=xxxx X=xxxx Y=xxxx A=xx B=xx CC=SXHINZVC";
// Where internal_read() stores what it read: outside, captured.
constexpr uint16_t SCRATCH = 0xFFA0;

// The offset of a BRA at |from| back to the park, the sequence's origin.
constexpr uint8_t bra(uint8_t from) {
    return static_cast<uint8_t>(-(from + 2));
}
}  // namespace

RegsMc68hc12::RegsMc68hc12(PinsMc68hc12 *pins, Mc68hc12Init &init)
    : _pins(pins), _init(init), _buffer(line) {}

const char *RegsMc68hc12::cpu() const {
    return "68HC12";
}

void RegsMc68hc12::print() const {
    _buffer.hex16(3, _pc);
    _buffer.hex16(11, _sp);
    _buffer.hex16(18, _x);
    _buffer.hex16(25, _y);
    _buffer.hex8(32, _a);
    _buffer.hex8(37, _b);
    _buffer.bits(43, _cc, 0x80, line + 43);
    cli.println(_buffer);
}

void RegsMc68hc12::reset() {
    // SP is undefined after reset; put it in external RAM.
    constexpr uint16_t sp = 0x2000;
    static constexpr uint8_t LDS[] = {
            InstMc68hc12::LDS_IMM,
            hi(sp),
            lo(sp),  // LDS #sp
            InstMc68hc12::BRA,
            bra(3),  // BRA park
    };
    _pins->execInst(LDS, sizeof(LDS));
}

void RegsMc68hc12::save() {
    // SWI stacks the context; its vector points back to the park.
    static constexpr uint8_t SWI[] = {InstMc68hc12::SWI};
    const auto park = _pins->park();
    const uint8_t vec[] = {hi(park), lo(park)};
    const PinsMc68hc12::Window win{InstMc68hc12::VEC_SWI, vec, sizeof(vec)};
    PinsMc68hc12::Capture cap;
    _pins->execInst(
            SWI, sizeof(SWI), &cap, FRAME, PinsMc68hc12::EXIT_PARK, &win);
    uint8_t frame[FRAME];
    const auto sp = cap.frame(frame, sizeof(frame));
    capture(sp, frame, true);
}

void RegsMc68hc12::capture(uint16_t sp, const uint8_t *frame, bool breakTrap) {
    _sp = sp + FRAME;
    _cc = frame[0];
    _b = frame[1];
    _a = frame[2];
    _x = be16(frame + 3);
    _y = be16(frame + 5);
    _pc = be16(frame + 7);
    if (breakTrap)
        --_pc;  // the SWI's address
}

void RegsMc68hc12::vectored(uint16_t pc) {
    _sp -= FRAME;
    _cc |= 0x10;  // I, as an interrupt sets it
    _pc = pc;
}

void RegsMc68hc12::restore() {
    _cc &= ~0x40;  // clear X bit to enable #XIRQ for step/suspend
    // The CPU builds the frame itself, so that a stack in internal RAM
    // works, and RTI takes it; an external one is answered from |frame|.
    // clang-format off
    const uint8_t PUSH_RTI[] = {
        InstMc68hc12::LDS_IMM, hi(_sp), lo(_sp),    // LDS #sp
        InstMc68hc12::LDD_IMM, hi(_pc), lo(_pc),    // LDD #pc
        InstMc68hc12::PSHD,                         // PSHD
        InstMc68hc12::LDD_IMM, hi(_y), lo(_y),      // LDD #y
        InstMc68hc12::PSHD,                         // PSHD
        InstMc68hc12::LDD_IMM, hi(_x), lo(_x),      // LDD #x
        InstMc68hc12::PSHD,                         // PSHD
        InstMc68hc12::LDAA_IMM, _a,                 // LDAA #a
        InstMc68hc12::PSHA,                         // PSHA
        InstMc68hc12::LDAA_IMM, _b,                 // LDAA #b
        InstMc68hc12::PSHA,                         // PSHA
        InstMc68hc12::LDAA_IMM, _cc,                // LDAA #cc
        InstMc68hc12::PSHA,                         // PSHA
        InstMc68hc12::RTI,                          // RTI
    };
    const uint8_t frame[FRAME] = {
        _cc, _b, _a, hi(_x), lo(_x), hi(_y), lo(_y), hi(_pc), lo(_pc),
    };
    // clang-format on
    const PinsMc68hc12::Window win{
            static_cast<uint16_t>(_sp - FRAME), frame, sizeof(frame)};
    PinsMc68hc12::Capture cap;
    _pins->execInst(PUSH_RTI, sizeof(PUSH_RTI), &cap, FRAME, _pc, &win);
}

uint8_t RegsMc68hc12::internal_read(uint16_t addr) const {
    const uint8_t LDAA_STAA[] = {
            InstMc68hc12::LDAA_EXT,
            hi(addr),
            lo(addr),  // LDAA addr
            InstMc68hc12::STAA_EXT,
            hi(SCRATCH),
            lo(SCRATCH),  // STAA scratch
            InstMc68hc12::BRA,
            bra(6),  // BRA park
    };
    PinsMc68hc12::Capture cap;
    _pins->execInst(LDAA_STAA, sizeof(LDAA_STAA), &cap, 1);
    return cap.n ? cap.data[0] : 0;
}

void RegsMc68hc12::internal_write(uint16_t addr, uint8_t data) const {
    const uint8_t LDAA_STAA[] = {
            InstMc68hc12::LDAA_IMM,
            data,  // LDAA #data
            InstMc68hc12::STAA_EXT,
            hi(addr),
            lo(addr),  // STAA addr
            InstMc68hc12::BRA,
            bra(5),  // BRA park
    };
    PinsMc68hc12::Capture cap;
    _pins->execInst(LDAA_STAA, sizeof(LDAA_STAA), &cap, 1);
}

void RegsMc68hc12::helpRegisters() const {
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

const Regs::RegList *RegsMc68hc12::listRegisters(uint_fast8_t n) const {
    static constexpr RegList REG_LIST[] = {
            {REGS8, 3, 1, UINT8_MAX},
            {REGS16, 5, 4, UINT16_MAX},
    };
    return n < 2 ? &REG_LIST[n] : nullptr;
}

bool RegsMc68hc12::setRegister(uint_fast8_t reg, uint32_t value) {
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
        _pc = value;
        return true;
    case 5:
        _sp = value;
        break;
    case 6:
        _x = value;
        break;
    case 7:
        _y = value;
        break;
    case 8:
        _d(value);
        break;
    }
    return false;
}

}  // namespace mc68hc12
}  // namespace debugger

// Local Variables:
// mode: c++
// c-basic-offset: 4
// tab-width: 4
// End:
// vim: set ft=cpp et ts=4 sw=4:
