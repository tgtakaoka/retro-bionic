#include "regs_i8096.h"
#include "debugger.h"
#include "inst_i8096.h"
#include "pins_i8096.h"

#include "signals_p8095.h"

namespace debugger {
namespace i8096 {

using p8095::Signals;

namespace {
// clang-format off
//                    012345678901234567890123456789012345
const char line1[] = "PC=xxxx SP=xxxx PSW=ZNVTC_IS11111111";
// clang-format on
}  // namespace

RegsI8096::RegsI8096(i8096::PinsI8096 *pins) : _pins(pins), _buffer1(line1) {}

const char *RegsI8096::cpu() const {
    return "I809610";
}

void RegsI8096::print() const {
    _buffer1.hex16(3, _pc);
    _buffer1.hex16(11, _sp);
    _buffer1.bits(20, _psw, 0x8000, line1 + 20);
    cli.println(_buffer1);
}

void RegsI8096::reset() {
    // Chip Configuration Register
    // 8 Bit width, #WR/#BHE, #ADV, infinite READY, no ROM protection
    _pins->injectRead(0xF5);
    // Load external address to SP
    // write_data16(ADDR_SP, 0x1234);
    write_data16(ADDR_SP, 0x1234);
}

void RegsI8096::save() {
    constexpr auto disp = -3;
    static constexpr uint8_t PUSHF[] = {
            InstI8096::PUSHF,
            InstI8096::SJMP(disp),  // SJMP $+2-3
            lo(disp),
            InstI8096::NOP,
    };
    _pc = _pins->injectReads(PUSHF, length(PUSHF));
    uint8_t buffer[2];
    _sp = _pins->captureWrites(buffer, sizeof(buffer));
    _sp += 2;
    _psw = le16(buffer);
    nops(2);
}

void RegsI8096::restore() {
    write_data16(ADDR_SP, _sp - 2);
    const uint8_t POPF[] = {
            InstI8096::POPF,
            InstI8096::NOP,
            InstI8096::NOP,
            InstI8096::NOP,
            lo(_psw),
            hi(_psw),
    };
    const auto pc = _pins->injectReads(POPF, length(POPF));
    const auto disp = _pc - (pc + 4 + 3);  // POP + NOPs + LJMP
    const uint8_t LJMP[] = {
            InstI8096::LJMP, lo(disp), hi(disp),  // LJMP _pc
    };
    _pins->injectReads(LJMP, length(LJMP));
    nops(2);
}

uint16_t RegsI8096::read_data(uint_fast8_t addr) const {
    if (Debugger.verbose()) {
        cli.print("read_data: addr=");
        cli.printlnHex(addr, 2);
        Signals::resetCycles();
    }
    constexpr auto disp = -7;
    constexpr auto abs = 0x5678;
    const uint8_t STB_ABS[] = {
            InstI8096::STB,  // STB addr 5678H[0]
            0x01,            // 16-bit indexed with [0]
            lo(abs),
            hi(abs),
            uint8(addr),
            InstI8096::SJMP(disp),  // SJMP $+2-7
            lo(disp),
    };
    _pins->injectReads(STB_ABS, sizeof(STB_ABS));
    uint8_t data;
    _pins->captureWrites(&data, sizeof(data));
    nops(3);
    if (Debugger.verbose())
        _pins->printCycles();
    return data;
}

void RegsI8096::write_data(uint_fast8_t addr, uint16_t data) const {
    if (Debugger.verbose()) {
        cli.print("write_data: addr=");
        cli.printHex(addr, 2);
        cli.print(" data=");
        cli.printlnHex(data, 2);
        Signals::resetCycles();
    }
    constexpr auto disp = -5;
    const uint8_t LDB_IM8[] = {
            InstI8096::LDB,  // LDB addr, #data
            lo(data),
            uint8(addr),
            InstI8096::SJMP(disp),  // SJMP $+2-5
            lo(disp),
    };
    _pins->injectReads(LDB_IM8, length(LDB_IM8));
    nops(2);
    if (Debugger.verbose())
        _pins->printCycles();
}

uint16_t RegsI8096::read_data16(uint_fast8_t addr) const {
    if (Debugger.verbose()) {
        cli.print("read_data16: addr=");
        cli.printlnHex(addr, 2);
        Signals::resetCycles();
    }
    constexpr auto disp = -7;
    constexpr auto abs = 0x5678;
    const uint8_t ST_ABS[] = {
            InstI8096::ST,  // ST addr 5678H[0]
            0x01,           // 16-bit indexed with [0]
            lo(abs),
            hi(abs),
            uint8(addr),
            InstI8096::SJMP(disp),  // SJMP $+2-7
            lo(disp),
    };
    _pins->injectReads(ST_ABS, sizeof(ST_ABS));
    uint8_t buffer[2];
    _pins->captureWrites(buffer, sizeof(buffer));
    nops(3);
    if (Debugger.verbose())
        _pins->printCycles();
    return le16(buffer);
}

void RegsI8096::write_data16(uint_fast8_t addr, uint16_t data) const {
    if (Debugger.verbose()) {
        cli.print("write_data16: addr=");
        cli.printHex(addr, 2);
        cli.print(" data=");
        cli.printlnHex(data, 4);
        Signals::resetCycles();
    }
    constexpr auto disp = -6;
    const uint8_t LD_IM16[] = {
            InstI8096::LD,  // LD addr, #data
            lo(data),
            hi(data),
            uint8(addr),
            InstI8096::SJMP(disp),  // SJMP $+2-6
            lo(disp),
    };
    _pins->injectReads(LD_IM16, length(LD_IM16));
    nops(2);
    if (Debugger.verbose())
        _pins->printCycles();
}

void RegsI8096::nops(uint_fast8_t len) const {
    static constexpr uint8_t NOPS[] = {
            InstI8096::NOP,
            InstI8096::NOP,
            InstI8096::NOP,
            InstI8096::NOP,
    };
    _pins->injectReads(NOPS, len);
}

void RegsI8096::helpRegisters() const {
    cli.println("?Reg: PC SP PSW");
}

constexpr const char *REGS16[] = {
        "PC",   // 1
        "SP",   // 2
        "PSW",  // 3
};

const Regs::RegList *RegsI8096::listRegisters(uint_fast8_t n) const {
    static constexpr RegList REG_LIST[] = {
            {REGS16, 3, 1, UINT16_MAX},
    };
    return n < 1 ? &REG_LIST[n] : nullptr;
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
