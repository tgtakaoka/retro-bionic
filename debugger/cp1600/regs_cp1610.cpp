#include "regs_cp1610.h"
#include "debugger.h"
#include "inst_cp1610.h"
#include "mems_cp1610.h"
#include "pins_cp1610.h"

namespace debugger {
namespace cp1600 {
namespace {
// clang-format off
//                              1         2         3         4         5         6
//                    0123456789012345678901234567890123456789012345678901234567890123456789
const char line[] = "R0=xxxx R1=xxxx R2=xxxx R3=xxxx R4=xxxx R5=xxxx SP=xxxx PC=xxxx SZOC I";
// clang-format on
constexpr char SW_BITS[] = "SZOC";
}  // namespace

RegsCp1610::RegsCp1610(PinsCp1610 *pins, MemsCp1610 *mems)
    : _pins(pins), _mems(mems), _buffer(line) {}

const char *RegsCp1610::cpu() const {
    return "CP1600";
}

const char *RegsCp1610::cpuName() const {
    return "CP1610";
}

void RegsCp1610::print() const {
    for (auto i = 0; i < 8; ++i)
        _buffer.hex16(i * 8 + 3, _r[i]);
    _buffer.bits(64, _sw, 0x80, SW_BITS);
    _buffer[69] = _intff ? 'I' : '_';
    cli.println(_buffer);
}

void RegsCp1610::reset() {
    for (auto &r : _r)
        r = 0;
    _sw = 0;
    _intff = false;  // #MSYNC disables interrupt
}

void RegsCp1610::save() {
    using I = InstCp1610;
    static constexpr uint16_t SAVE[] = {
            I::mvo(0),
            0,           // MVO R0, 0
            I::gswd(0),  // GSWD R0
            I::mvo(0),
            0,  // MVO R0, 0
            I::mvo(1),
            0,  // MVO R1, 0
            I::mvo(2),
            0,  // MVO R2, 0
            I::mvo(3),
            0,  // MVO R3, 0
            I::mvo(4),
            0,  // MVO R4, 0
            I::mvo(5),
            0,  // MVO R5, 0
            I::mvo(6),
            0,  // MVO R6, 0
    };
    uint16_t buf[8];
    _r[PC] = _pins->injectInsts(SAVE, sizeof(SAVE) / sizeof(SAVE[0]), buf, 8);
    _r[0] = buf[0];
    _sw = buf[1] & 0xF0;
    for (auto i = 1; i <= SP; ++i)
        _r[i] = buf[i + 1];
    _intff = _pins->probeIntff();
}

void RegsCp1610::restore() {
    using I = InstCp1610;
    const auto pc = _r[PC];
    const uint16_t RESTORE[] = {
            I::mvii(0),
            _sw,         // MVII sw, R0
            I::rswd(0),  // RSWD R0
            I::mvii(0),
            _r[0],  // MVII r0, R0
            I::mvii(1),
            _r[1],  // MVII r1, R1
            I::mvii(2),
            _r[2],  // MVII r2, R2
            I::mvii(3),
            _r[3],  // MVII r3, R3
            I::mvii(4),
            _r[4],  // MVII r4, R4
            I::mvii(5),
            _r[5],  // MVII r5, R5
            I::mvii(6),
            _r[SP],  // MVII sp, R6
            I::J,
            I::j2(pc, _intff ? 1 : 2),
            I::j3(pc),  // JE/JD pc
    };
    _pins->resumeInsts(RESTORE, sizeof(RESTORE) / sizeof(RESTORE[0]));
}

void RegsCp1610::interrupted(uint16_t pc, uint16_t sp) {
    _r[PC] = pc;
    _r[SP] = sp;
}

void RegsCp1610::helpRegisters() const {
    cli.println(F("?Reg: R0~R7 SP PC SW IE"));
}

constexpr const char *REGS16[] = {
        "R0",  // 1
        "R1",  // 2
        "R2",  // 3
        "R3",  // 4
        "R4",  // 5
        "R5",  // 6
        "R6",  // 7
        "R7",  // 8
        "SP",  // 9
        "PC",  // 10
};

constexpr const char *REGS8[] = {
        "SW",  // 11
};

constexpr const char *REGS1[] = {
        "IE",  // 12
};

const Regs::RegList *RegsCp1610::listRegisters(uint_fast8_t n) const {
    static constexpr RegList CP1610_LIST[] = {
            {REGS16, 10, 1, UINT16_MAX},
            {REGS8, 1, 11, UINT8_MAX},
            {REGS1, 1, 12, 1},
    };
    return n < 3 ? &CP1610_LIST[n] : nullptr;
}

bool RegsCp1610::setRegister(uint_fast8_t reg, uint32_t value) {
    if (reg <= 8) {
        _r[reg - 1] = value;
    } else if (reg <= 10) {
        _r[reg - 9 + SP] = value;
    } else if (reg == 11) {
        _sw = value & 0xF0;
    } else {
        _intff = value != 0;
    }
    return reg == 8 || reg == 10;  // R7/PC
}

}  // namespace cp1600
}  // namespace debugger

// Local Variables:
// mode: c++
// c-basic-offset: 4
// tab-width: 4
// End:
// vim: set ft=cpp et ts=4 sw=4:
