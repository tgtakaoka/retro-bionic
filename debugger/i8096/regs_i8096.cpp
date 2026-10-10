#include "regs_i8096.h"
#include "debugger.h"
#include "inst_i8096.h"
#include "pins_i8096.h"

#include "signals_i8096.h"

namespace debugger {
namespace i8096 {

namespace {
// clang-format off
//                    012345678901234567890123456789012345
const char line1[] = "PC=xxxx SP=xxxx PSW=ZNVTC_IS11111111";
//                    0123456789012345
const char line2[] = " IM1=xx WSR=xx";
// clang-format on
}  // namespace

RegsI8096::RegsI8096(i8096::PinsI8096 *pins)
    : _pins(pins), _buffer1(line1), _buffer2(line2) {}

// libasm's names for the instruction sets.
const char *RegsI8096::cpu() const {
    switch (_pins->cpuType()) {
    case CPU_80C196KC:
        return "80196";
    case CPU_80C196KB:
        return "80196KB";
    default:
        return "8096";
    }
}

void RegsI8096::print() const {
    _buffer1.hex16(3, _pc);
    _buffer1.hex16(11, _sp);
    _buffer1.bits(20, _psw, 0x8000, line1 + 20);
    if (_pins->c196()) {
        _buffer2.hex8(5, _int_mask1);
        _buffer2.hex8(12, _wsr);
        cli.print(_buffer1);
        cli.println(_buffer2);
    } else {
        cli.println(_buffer1);
    }
}

void RegsI8096::reset() {
    // Load external address to SP
    write_data16(ADDR_SP, 0x1234);
    // A reset leaves INT_PENDING undefined, as SP: a stale EXTINT would
    // be taken once a program enables it.
    write_data(ADDR_INT_PENDING, 0);
    if (_pins->c196())
        write_data(ADDR_INT_PENDING1, 0);
}

// Each sequence runs where the CPU is parked and jumps back there. The
// 80C196's PUSHA also pushes INT_MASK1 and WSR; WSR is cleared so the
// debugger sees the register file unwindowed.
void RegsI8096::save() {
    static constexpr uint8_t PUSHF[] = {
            0xF2,      // PUSHF
            SJMP(-3),  // SJMP $+2-3
    };
    static constexpr uint8_t PUSHA[] = {
            0xF4,               // PUSHA
            0xB1, 0, ADDR_WSR,  // LDB WSR, #0
            SJMP(-6),           // SJMP $+2-6
    };
    _pc = _pins->park();
    uint8_t buffer[4];
    if (_pins->c196()) {
        _sp = _pins->execInst(PUSHA, length(PUSHA), buffer, 4);
        _int_mask1 = buffer[2];
        _wsr = buffer[3];
    } else {
        _sp = _pins->execInst(PUSHF, length(PUSHF), buffer, 2);
    }
    _sp += 2;
    _psw = le16(buffer);
}

void RegsI8096::captureContext(uint16_t sp, uint16_t pc, bool breakTrap) {
    save();
    _sp = sp + 2;
    _pc = pc;
    if (breakTrap)
        --_pc;
}

void RegsI8096::restore() {
    const auto disp = _pc - (_pins->park() + 4);  // POPF/POPA + LJMP
    if (_pins->c196()) {
        write_data16(ADDR_SP, _sp - 4);
        const uint8_t POPA[] = {
                0xF5,                      // POPA
                0xE7, lo(disp), hi(disp),  // LJMP _pc
        };
        const uint8_t words[] = {_int_mask1, _wsr, lo(_psw), hi(_psw)};
        _pins->popInst(POPA, length(POPA), _sp - 4, words, sizeof(words), _pc);
    } else {
        write_data16(ADDR_SP, _sp - 2);
        const uint8_t POPF[] = {
                0xF3,                      // POPF
                0xE7, lo(disp), hi(disp),  // LJMP _pc
        };
        const uint8_t psw[] = {lo(_psw), hi(_psw)};
        _pins->popInst(POPF, length(POPF), _sp - 2, psw, sizeof(psw), _pc);
    }
}

// The 80C196KC's register RAM above FFH takes indexed addressing: a word
// pushed from it, or popped to it, through a stack moved out to the bus.
// restore() puts SP back.
uint16_t RegsI8096::read_upper16(uint16_t addr) const {
    constexpr auto disp = -10;
    constexpr auto stack = 0x5678;
    const uint8_t PUSH_ABS[] = {
            0xA1, lo(stack), hi(stack), ADDR_SP,  // LD SP, #5678H
            0xCB, 0x01, lo(addr), hi(addr),       // PUSH addr[0]
            SJMP(disp),                           // SJMP $+2-10
    };
    uint8_t buffer[2];
    _pins->execInst(PUSH_ABS, length(PUSH_ABS), buffer, sizeof(buffer));
    return le16(buffer);
}

void RegsI8096::write_upper16(uint16_t addr, uint16_t data) const {
    constexpr auto disp = -10;
    constexpr auto stack = 0x5678;
    const uint8_t POP_ABS[] = {
            0xA1, lo(stack), hi(stack), ADDR_SP,  // LD SP, #5678H
            0xCF, 0x01, lo(addr), hi(addr),       // POP addr[0]
            SJMP(disp),                           // SJMP $+2-10
    };
    const uint8_t word[] = {lo(data), hi(data)};
    _pins->popInst(POP_ABS, length(POP_ABS), stack, word, sizeof(word),
            PinsI8096::EXIT_PARK);
}

uint16_t RegsI8096::read_data(uint16_t addr) const {
    if (addr >= 0x100) {
        const auto word = read_upper16(addr & ~1);
        return (addr & 1) ? hi(word) : lo(word);
    }
    constexpr auto disp = -7;
    constexpr auto abs = 0x5678;
    const uint8_t STB_ABS[] = {
            0xC7, 0x01, lo(abs), hi(abs), lo(addr),  // STB addr, 5678H[0]
            SJMP(disp),                          // SJMP $+2-7
    };
    uint8_t data;
    _pins->execInst(STB_ABS, length(STB_ABS), &data, sizeof(data));
    return data;
}

void RegsI8096::write_data(uint16_t addr, uint16_t data) const {
    if (addr >= 0x100) {
        const uint16_t even = addr & ~1;
        auto word = read_upper16(even);
        if (addr & 1) {
            word = uint16(data, lo(word));
        } else {
            word = uint16(hi(word), data);
        }
        write_upper16(even, word);
        return;
    }
    constexpr auto disp = -5;
    const uint8_t LDB_IM8[] = {
            0xB1, lo(data), lo(addr),  // LDB addr, #data
            SJMP(disp),            // SJMP $+2-5
    };
    _pins->execInst(LDB_IM8, length(LDB_IM8));
}

uint16_t RegsI8096::read_data16(uint8_t addr) const {
    constexpr auto disp = -7;
    constexpr auto abs = 0x5678;
    const uint8_t ST_ABS[] = {
            0xC3, 0x01, lo(abs), hi(abs), addr,  // ST addr, 5678H[0]
            SJMP(disp),                          // SJMP $+2-7
    };
    uint8_t buffer[2];
    _pins->execInst(ST_ABS, length(ST_ABS), buffer, sizeof(buffer));
    return le16(buffer);
}

void RegsI8096::write_data16(uint8_t addr, uint16_t data) const {
    constexpr auto disp = -6;
    const uint8_t LD_IM16[] = {
            0xA1, lo(data), hi(data), addr,  // LD addr, #data
            SJMP(disp),                      // SJMP $+2-6
    };
    _pins->execInst(LD_IM16, length(LD_IM16));
}

void RegsI8096::helpRegisters() const {
    cli.println(_pins->c196() ? "?Reg: PC SP PSW IM1 WSR" : "?Reg: PC SP PSW");
}

constexpr const char *REGS16[] = {
        "PC",   // 1
        "SP",   // 2
        "PSW",  // 3
};

constexpr const char *REGS8[] = {
        "IM1",  // 4
        "WSR",  // 5
};

const Regs::RegList *RegsI8096::listRegisters(uint_fast8_t n) const {
    static constexpr RegList REG_LIST[] = {
            {REGS16, 3, 1, UINT16_MAX},
            {REGS8, 2, 4, UINT8_MAX},
    };
    return n < (_pins->c196() ? 2 : 1) ? &REG_LIST[n] : nullptr;
}

bool RegsI8096::setRegister(uint_fast8_t reg, uint32_t value) {
    switch (reg) {
    case 1:
        _pc = value;
        return true;
    case 2:
        _sp = value;
        break;
    case 3:
        _psw = value;
        break;
    case 4:
        _int_mask1 = value;
        break;
    case 5:
        _wsr = value;
        break;
    default:
        break;
    }
    return false;
}

}  // namespace i8096
}  // namespace debugger

// Local Variables:
// mode: c++
// c-basic-offset: 4
// tab-width: 4
// End:
// vim: set ft=cpp et ts=4 sw=4:
