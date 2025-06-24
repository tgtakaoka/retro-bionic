#include "regs_tms370.h"
#include "debugger.h"
#include "pins_tms370.h"

namespace debugger {
namespace tms370 {

namespace {
//                   01234567890123456789012345678901234
const char line[] = "PC=xxxx SP=xx A=xx B=xx ST=CNZV11__";
}  // namespace

RegsTms370::RegsTms370(PinsTms370 *pins) : _pins(pins), _buffer(line) {}

const char *RegsTms370::cpu() const {
    return "TMS370";
}

void RegsTms370::print() const {
    _buffer.hex16(3, _pc);
    _buffer.hex8(11, _sp);
    _buffer.hex8(16, _a);
    _buffer.hex8(21, _b);
    _buffer.bits(27, _st, 0x80, line + 27);
    cli.println(_buffer);
    _pins->idle();
}

void RegsTms370::reset() {
    // set SCCR1.4 (AITO-WAIT DISABLE)
    // set SCCR2.1 (INT1 NMI)
    constexpr auto DUMMY = 0x2000;
    static constexpr uint8_t CONFIG[] = {
            lo(DUMMY), hi(DUMMY),  // inject dummy reset vector
            0xF7, 0x60, 0x10,      // MOV #OSC_POWER|PF_AUTOWAOT,P010
            0xF7, 0x10, 0x11,      // MOV #AUTO_WAIT_DISABLE, P011
    };
    _pins->injectReads(CONFIG, sizeof(CONFIG));
}

void RegsTms370::save() {
    static constexpr uint8_t SAVE_ALL[] = {
            0xFB, 0,           // PUSH ST
            0x8B, 0x80, 0x00,  // MOV A, >8000
            0xFC, 0,           // POP A
            0x8B, 0x80, 0x00,  // MOV A, >8000
            0x62, 0,           // MOV B, A
            0x8B, 0x80, 0x00,  // MOV A, >8000
            0xFE, 0,           // STSP
            0x62, 0,           // MOV B, A
            0x8B, 0x80, 0x00,  // MOV A, >8000
    };
    uint8_t buffer[4];
    _pc = _pins->captureWrites(
            SAVE_ALL, sizeof(SAVE_ALL), buffer, sizeof(buffer));
    _a = buffer[0];
    _st = buffer[1];
    _b = buffer[2];
    _sp = buffer[3];
}

void RegsTms370::restore() {
    const uint8_t LOAD_ALL[] = {
            0x52, _sp,               // MOV #_sp, B
            0xFD, 0,                 // LDSP
            0x52, _b,                // MOV #_b, B
            0x22, _st,               // MOV #_st, A
            0xFB, 0,                 // PUSH A
            0x22, _a,                // MOV #_a, A
            0xFC, 0,                 // POP ST
            0x8C, hi(_pc), lo(_pc),  // BR _pc
    };
    _pins->injectReads(LOAD_ALL, sizeof(LOAD_ALL));
}

void RegsTms370::restoreA() {
    write_internal(A, _a);
}

uint_fast8_t RegsTms370::read_internal(uint16_t addr) {
    const uint8_t READ[] = {
            0x8A, hi(addr), lo(addr),  // MOV addr, A
    };
    _pins->injectReads(READ, sizeof(READ), 1);
    const uint8_t CAPTURE[] = {
            0x8B, 0x80, 0x00,  // MOV A, >8000
    };
    uint8_t data;
    _pins->captureWrites(CAPTURE, sizeof(CAPTURE), &data, sizeof(data));
    restoreA();
    return data;
}

void RegsTms370::write_internal(uint16_t addr, uint8_t data) {
    if (addr == A) {
        const uint8_t MOV_A[] = {
                0x22, _a = data,  // MOV #data, A
        };
        _pins->injectReads(MOV_A, sizeof(MOV_A));
        return;
    }
    if (addr == B) {
        const uint8_t MOV_B[] = {
                0x52, _b = data,  // MOV #data, B
        };
        _pins->injectReads(MOV_B, sizeof(MOV_B));
        return;
    }
    const uint8_t WRITE[] = {
            0x22, data,                // MOV #data, A
            0x8B, hi(addr), lo(addr),  // MOV A, addr
    };
    _pins->injectReads(WRITE, sizeof(WRITE), 1);
    restoreA();
}

void RegsTms370::helpRegisters() const {
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

const Regs::RegList *RegsTms370::listRegisters(uint_fast8_t n) const {
    static constexpr RegList REG_LIST[] = {
            {REGS8, 4, 1, UINT8_MAX},
            {REGS16, 1, 5, UINT16_MAX},
    };
    return n < 2 ? &REG_LIST[n] : nullptr;
}

bool RegsTms370::setRegister(uint_fast8_t reg, uint32_t value) {
    switch (reg) {
    case 5:
        _pc = value;
        return true;
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
    return false;
}

}  // namespace tms370
}  // namespace debugger

// Local Variables:
// mode: c++
// c-basic-offset: 4
// tab-width: 4
// End:
// vim: set ft=cpp et ts=4 sw=4:
