#include "regs_mn1613.h"
#include "debugger.h"
#include "inst_mn1613.h"
#include "mems_mn1613.h"
#include "pins_mn1613.h"

namespace debugger {
namespace mn1613 {
namespace {
// clang-format off
//                              1         2         3         4         5         6
//                    012345678901234567890123456789012345678901234567890123456789012
const char line1[] = "R0=xxxx R1=xxxx R2=xxxx X0=xxxx X1=xxxx    STR=E_V__012########";
const char line2[] = "IC=xxxx SP=xxxx CSBR=xx SSBR=xx TSR0=xx TSR1=xx NPP=XXXX IISR=X";
// clang-format on
}  // namespace

RegsMn1613::RegsMn1613(PinsMn1613 *pins, MemsMn1613 *mems)
    : _pins(pins),
      _mems(mems),
      _mn1613A(false),
      _buffer1(line1),
      _buffer2(line2) {}

const char *RegsMn1613::cpu() const {
    return _mn1613A ? "MN1613A" : "MN1613";
}

const char *RegsMn1613::cpuName() const {
    return cpu();
}

void RegsMn1613::print() const {
    for (auto rs = R0; rs < SP; ++rs)
        _buffer1.hex16(rs * 8 + 3, _reg[rs]);
    _buffer1.bits(47, _reg[STR], 0x8000, line1 + 47);
    cli.println(_buffer1);
    _buffer2.hex16(3, _reg[IC]);
    _buffer2.hex16(11, _reg[SP]);
    for (auto b = CSBR; b < 4; ++b)
        _buffer2.hex8(b * 8 + 21, _seg[b]);
    _buffer2.hex16(52, _npp);
    _buffer2.hex1(62, _iisr);
    cli.println(_buffer2);
}

void RegsMn1613::reset() {
    const auto reset_str = _mems->read(InstMn1613::RESET_PSW);
    const auto reset_ic = _mems->read(InstMn1613::RESET_PSW + 1);
    const uint16_t RESET_PSW[] = {
            InstMn1613::RESET_PSW,
            reset_str,
            reset_ic,
    };
    _pins->injectReset(RESET_PSW, length(RESET_PSW));  // Inject reset PSW
}

uint32_t RegsMn1613::addr(uint_fast8_t seg, uint16_t off) {
    return (static_cast<uint32_t>(seg) << 14) | off;
}

uint32_t RegsMn1613::nextIp() const {
    return addr(_seg[CSBR], _reg[IC]);
}

uint32_t RegsMn1613::addIp(int_fast8_t n) const {
    return addr(_seg[CSBR], _reg[IC] + n);
}

void RegsMn1613::setIp(uint32_t pc) {
    pc -= static_cast<uint32_t>(_seg[CSBR]) << 14;
    _reg[IC] = pc;
}

void RegsMn1613::save() {
    /* Save registers */
    for (uint_fast8_t rs = R0; rs < length(_reg); ++rs)
        _reg[rs] = getReg(rs);
    _reg[IC] -= length(_reg);  // offset getReg instructions
    /* Save segment registers */
    for (uint_fast8_t b = CSBR; b < 4; ++b) {
        const uint16_t CPYB = 0x0F80 | (b << 4) | R0;  // CPYB R0, BRb
        _seg[b] = captureReg(CPYB);
    }
    /* Save NPP */
    const uint16_t CPYS = 0x0F88 | (NPP << 4) | R0;  // CPYS R0, NPP
    _npp = captureReg(CPYS);
    /* Save IISR */
    const uint16_t CPYH = 0x3F80 | (IISR << 4) | R0;  // CPYH R0, IISR
    _iisr = captureReg(CPYH);
}

void RegsMn1613::restore() {
    /* Set known value to IC. */
    _pins->injectInst(0xC780);  // B X'0080'
    /* Restore IISR */
    const uint16_t SETH = 0x3F00 | (IISR << 4) | R0;  // SETH R0, IISR
    injectReg(SETH, _iisr);
    /* Restore NPP */
    const uint16_t SETS = 0x0F08 | (NPP << 4) | R0;  // SETS R0, NPP
    injectReg(SETS, _npp);
    /* Restore SSBR, TSR0, TSR1 */
    for (uint_fast8_t b = SSBR; b < 4; ++b) {
        const uint16_t SETB = 0x0F00 | (b << 4) | R0;  // SETB R0, RBb
        injectReg(SETB, _seg[b]);
    }
    /* Restore R0~R4 */
    for (uint_fast8_t rd = R0; rd < SP; rd++)
        setReg(rd, _reg[rd]);
    /* Set known value to SP */
    const auto KNOWN_SP = UINT16_C(0x1000);
    setReg(SP, KNOWN_SP);
    /* Restore STR; POP STR */
    auto sp = addr(_seg[SSBR], KNOWN_SP);
    _pins->injectReads(0x2602, ++sp, &_reg[STR], 1);
    /* Rsetore CSBR:IC */
    const uint16_t ADDR[] = {
            _seg[CSBR],
            uint16(_reg[IC] - 2),  // offset setReg(SP) instructions
    };
    _pins->injectReads(0x3F07, ++sp, ADDR, length(ADDR));  // RETL
    /* Restore SP */
    setReg(SP, _reg[SP]);
}

uint16_t RegsMn1613::getReg(uint_fast8_t rs) const {
    // R0-R4: ST   Rs, X'0000'
    // SP:    ST   SP, X'0000'
    // STR:   PUSH STR
    // IC:    BAL  X'0000'
    const uint16_t inst = (rs == STR) ? 0x2601 : (0x8000 | (rs << 8));
    uint16_t data;
    _pins->captureInst(inst, &data);
    return data;
}

void RegsMn1613::setReg(uint_fast8_t rd, uint16_t data) const {
    const uint16_t MVWI[] = {
            uint16(0x780F | (rd << 8)),
            data,
    };
    _pins->injectInst(MVWI, length(MVWI));  // MVWI Rd, data
}

uint16_t RegsMn1613::captureReg(uint16_t inst) const {
    _pins->injectInst(inst);  // inst; reg->R0
    return getReg(R0);
}

void RegsMn1613::injectReg(uint16_t inst, uint16_t data) const {
    setReg(R0, data);         // MVWI R0, data
    _pins->injectInst(inst);  // inst; R0->reg
}

void RegsMn1613::checkCpuType() {
    constexpr uint16_t SETB = 0x0F00 | (TSR0 << 4) | R0;  // SETB R0, TSR0
    constexpr uint16_t CPYB = 0x0F80 | (TSR0 << 4) | R0;  // CPYB R0, TSR0
    injectReg(SETB, 0x3F);                                // write X'3F' to TSR0
    _mn1613A = captureReg(CPYB) == 0x3F;                  // check TSR0
    _mems->setMaxAddr(_mn1613A ? 0xFFFFF : 0x3FFFF);
}

void RegsMn1613::helpRegisters() const {
    cli.println(F("?Reg: IC SP STR R0~R4 X0 X1 CSBR SSBR TSR0 TSR1 NPP IISR"));
}

constexpr const char *REGS16[] = {
        "R0",   // 1
        "R1",   // 2
        "R2",   // 3
        "R3",   // 4
        "R4",   // 5
        "SP",   // 6
        "STR",  // 7
        "IC",   // 8
        "X0",   // 9
        "X1",   // 10
        "NPP",  // 11
};

constexpr const char *SEGS[] = {
        "CSBR",  // 12
        "SSBR",  // 13
        "TSR0",  // 14
        "TSR1",  // 15
};

constexpr const char *REGS1[] = {
        "IISR",  // 16
};

const Regs::RegList *RegsMn1613::listRegisters(uint_fast8_t n) const {
    static constexpr RegList MN1613_LIST[] = {
            {REGS16, 11, 1, UINT16_MAX},
            {SEGS, 4, 12, 0x0F},
            {REGS1, 1, 16, 1},
    };
    static constexpr RegList MN1613A_LIST[] = {
            {REGS16, 11, 1, UINT16_MAX},
            {SEGS, 4, 12, 0x3F},
            {REGS1, 1, 16, 1},
    };
    const auto list = _mn1613A ? MN1613A_LIST : MN1613_LIST;
    return n < 3 ? &list[n] : nullptr;
}

bool RegsMn1613::setRegister(uint_fast8_t reg, uint32_t value) {
    switch (reg) {
    case 9:   // X0=R3
    case 10:  // X1=R4
        _reg[reg - 9 + 3] = value;
        break;
    case 11:
        _npp = value & 0xFF00;
        break;
    case 16:
        _iisr = value;
    default:
        if (reg < 9) {
            _reg[reg - 1] = value;
        } else {
            _seg[reg - 12] = value;
        }
        break;
    }
    return reg == 8 || reg == 12;  // IC/CSBR
}

}  // namespace mn1613
}  // namespace debugger

// Local Variables:
// mode: c++
// c-basic-offset: 4
// tab-width: 4
// End:
// vim: set ft=cpp et ts=4 sw=4:
