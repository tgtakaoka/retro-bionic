#include "regs_tms7000.h"
#include "char_buffer.h"
#include "debugger.h"
#include "digital_bus.h"
#include "inst_tms7000.h"
#include "mems_tms7000.h"
#include "pins_tms7000.h"

namespace debugger {
namespace tms7000 {

struct RegsTms7000 Regs;

namespace {
constexpr const char *const CPU_NAMES[/*HardwareType*/] = {
        "TMS7000NL2",  // 0
        "TMS7001NL2",  // 1
        "TMS7002",     // 2
        "TMS70C02",    // 3
        "TMS7000NL4",  // 0+4
        "TMS7001NL4",  // 1+4
};
}

const char *RegsTms7000::cpu() const {
    return CPU_NAMES[HW_TMS7000];
}

const char *RegsTms7000::cpuName() const {
    const auto type = Pins.hardwareType() + Pins.clockType();
    return CPU_NAMES[type];
}

void RegsTms7000::print() const {
    //                              01234567890123456789012345678901234
    static constexpr char line[] = "PC=xxxx SP=xx A=xx B=xx ST=CNZI1111";
    static auto &buffer = *new CharBuffer(line);
    buffer.hex16(3, _pc);
    buffer.hex8(11, _sp);
    buffer.hex8(16, _a);
    buffer.hex8(21, _b);
    buffer.bits(27, _st, 0x80, line + 27);
    cli.println(buffer);
    Pins.idle();
}

void RegsTms7000::save() {
    static constexpr uint8_t SAVE_ALL[] = {
            0x0E,              // PUSH ST
            0x8B, 0x80, 0x00,  // STA @>8000
            0xB9,              // POP A
            0x8B, 0x80, 0x00,  // STA @>8000
            0x62,              // MOV B, A
            0x8B, 0x80, 0x00,  // STA @>8000
            0x09,              // STSP
            0x62,              // MOV B, A
            0x8B, 0x80, 0x00,  // STA @>8000
    };
    uint8_t buffer[4];
    Pins.captureWrites(
            SAVE_ALL, sizeof(SAVE_ALL), buffer, sizeof(buffer), &_pc);
    write_internal(A, buffer[0]);
    _st = buffer[1];
    write_internal(B, buffer[2]);
    _sp = buffer[3];
}

void RegsTms7000::restore() {
    uint8_t LOAD_ALL[] = {
            0x52, _sp,               // MOV %_sp, B
            0x0D,                    // LDSP
            0x52, _b,                // MOV %_b, B
            0x22, _st,               // MOV %_st, A
            0xB8,                    // PUSH A
            0x22, _a,                // MOV %_a, A
            0x08,                    // POP ST
            0x8C, hi(_pc), lo(_pc),  // BR @_pc
    };
    Pins.execInst(LOAD_ALL, sizeof(LOAD_ALL));
}

void RegsTms7000::restoreA() {
    write_internal(A, _a);
}

uint8_t RegsTms7000::read_internal(uint16_t addr) {
    uint8_t READ[] = {
            0x8A, hi(addr), lo(addr),  // LDA @addr
    };
    Pins.execInst(READ, sizeof(READ), 1);
    uint8_t CAPTURE[] = {
            0x8B, 0x80, 0x00,  // STA @>8000
    };
    uint8_t data;
    Pins.captureWrites(CAPTURE, sizeof(CAPTURE), &data, sizeof(data));
    restoreA();
    return data;
}

void RegsTms7000::write_internal(uint16_t addr, uint8_t data) {
    if (addr == A) {
        uint8_t MOV_A[] = {
                0x22, _a = data,  // MOV %data, A
        };
        Pins.execInst(MOV_A, sizeof(MOV_A));
        return;
    }
    if (addr == B) {
        uint8_t MOV_B[] = {
                0x52, _b = data,  // MOV %data, B
        };
        Pins.execInst(MOV_B, sizeof(MOV_B));
        return;
    }
    uint8_t WRITE[] = {
            0x22, data,                // MOV %data, A
            0x8B, hi(addr), lo(addr),  // STA @addr
    };
    Pins.execInst(WRITE, sizeof(WRITE), 1);
    restoreA();
}

void RegsTms7000::helpRegisters() const {
    cli.println(F("?Reg: PC SP A B ST"));
}

constexpr const char *REGS8[] = {
        "ST",  // 1
        "A",   // 2
        "B",   // 3
        "SP",  // 4
};
constexpr const char *REGS16[] = {
        "PC",  // 5
};

const Regs::RegList *RegsTms7000::listRegisters(uint8_t n) const {
    static constexpr RegList REG_LIST[] = {
            {REGS8, 4, 1, UINT8_MAX},
            {REGS16, 1, 5, UINT16_MAX},
    };
    return n < 2 ? &REG_LIST[n] : nullptr;
}

void RegsTms7000::setRegister(uint8_t reg, uint32_t value) {
    switch (reg) {
    case 5:
        _pc = value;
        break;
    case 4:
        _sp = value;
        break;
    case 2:
        write_internal(A, value);
        break;
    case 3:
        write_internal(B, value);
        break;
    case 1:
        _st = value;
        break;
    }
}

}  // namespace tms7000
}  // namespace debugger

// Local Variables:
// mode: c++
// c-basic-offset: 4
// tab-width: 4
// End:
// vim: set ft=cpp et ts=4 sw=4:
