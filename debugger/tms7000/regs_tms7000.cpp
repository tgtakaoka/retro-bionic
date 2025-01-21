#include "regs_tms7000.h"
#include "debugger.h"
#include "pins_tms7000.h"

namespace debugger {
namespace tms7000 {

namespace {
const char *const CPU_NAMES[/*HardwareType*/] = {
        "TMS7000",     // 0+0
        "TMS7000NL4",  // 0+1
        "TMS70C00",    // 0+2
        "TMS7001",     // 3+0
        "TMS7001NL4",  // 3+1
        "",            // no TMS70C01
        "TMS7002",     // 6+0
        "",            // no TMS7002NL4
        "TMS70C02",    // 6+2
};

//                   01234567890123456789012345678901234
const char line[] = "PC=xxxx SP=xx A=xx B=xx ST=CNZ#####";
}  // namespace

RegsTms7000::RegsTms7000(PinsTms7000 *pins) : _pins(pins), _buffer(line) {}

const char *RegsTms7000::cpu() const {
    return CPU_NAMES[HW_TMS7000];
}

const char *RegsTms7000::cpuName() const {
    const auto type = _pins->hardwareType() + _pins->clockType();
    return CPU_NAMES[type];
}

void RegsTms7000::print() const {
    _buffer.hex16(3, _pc);
    _buffer.hex8(11, _sp);
    _buffer.hex8(16, _a);
    _buffer.hex8(21, _b);
    _buffer.bits(27, _st, 0x80, line + 27);
    cli.println(_buffer);
    _pins->idle();
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
    _pins->captureWrites(
            SAVE_ALL, sizeof(SAVE_ALL), buffer, sizeof(buffer), &_pc);
    write_internal(A, buffer[0]);
    _st = buffer[1];
    write_internal(B, buffer[2]);
    _sp = buffer[3];
}

void RegsTms7000::restore() {
    const uint8_t LOAD_ALL[] = {
            0x52, _sp,               // MOV %_sp, B
            0x0D,                    // LDSP
            0x52, _b,                // MOV %_b, B
            0x22, _st,               // MOV %_st, A
            0xB8,                    // PUSH A
            0x22, _a,                // MOV %_a, A
            0x08,                    // POP ST
            0x8C, hi(_pc), lo(_pc),  // BR @_pc
    };
    _pins->execInst(LOAD_ALL, sizeof(LOAD_ALL));
}

void RegsTms7000::restoreA() {
    write_internal(A, _a);
}

uint8_t RegsTms7000::read_internal(uint16_t addr) {
    const uint8_t READ[] = {
            0x8A, hi(addr), lo(addr),  // LDA @addr
    };
    _pins->execInst(READ, sizeof(READ), 1);
    const uint8_t CAPTURE[] = {
            0x8B, 0x80, 0x00,  // STA @>8000
    };
    uint8_t data;
    _pins->captureWrites(CAPTURE, sizeof(CAPTURE), &data, sizeof(data));
    restoreA();
    return data;
}

void RegsTms7000::write_internal(uint16_t addr, uint8_t data) {
    if (addr == A) {
        const uint8_t MOV_A[] = {
                0x22, _a = data,  // MOV %data, A
        };
        _pins->execInst(MOV_A, sizeof(MOV_A));
        return;
    }
    if (addr == B) {
        const uint8_t MOV_B[] = {
                0x52, _b = data,  // MOV %data, B
        };
        _pins->execInst(MOV_B, sizeof(MOV_B));
        return;
    }
    const uint8_t WRITE[] = {
            0x22, data,                // MOV %data, A
            0x8B, hi(addr), lo(addr),  // STA @addr
    };
    _pins->execInst(WRITE, sizeof(WRITE), 1);
    restoreA();
}

void RegsTms7000::helpRegisters() const {
    cli.println("?Reg: PC SP A B ST");
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
