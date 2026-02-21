#include "regs_pdp8.h"
#include "debugger.h"
#include "inst_pdp8.h"
#include "pins_pdp8.h"

namespace debugger {
namespace pdp8 {

RegsPdp8::RegsPdp8(PinsPdp8 *pins) : _pins(pins) {}

const char *RegsPdp8::cpuName() const {
    return Debugger.target().identity();
}

void RegsPdp8::reset() {
    _pc = pdp8::InstPdp8::ORG_RESET;
}

constexpr const char *REGS12[] = {
        "AC",  // REG_AC
        "MQ",  // REG_MQ
        "PC",  // REG_PC
        "SW",  // REG_SW
};
constexpr const char *REGS1[] = {
        "L",     // REG_LINK
        "IEFF",  // REG_IEFF
};

const Regs::RegList *RegsPdp8::listRegisters(uint_fast8_t n) const {
    static constexpr RegList REG_LIST[] = {
            {REGS12, 4, REG_AC, 07777},
            {REGS1, 2, REG_LINK, 1},
    };
    return n < 2 ? &REG_LIST[n] : nullptr;
}

bool RegsPdp8::setRegister(uint_fast8_t reg, uint32_t value) {
    switch (reg) {
    case REG_AC:
        _ac = value;
        break;
    case REG_MQ:
        _mq = value;
        break;
    case REG_PC:
        _pc = value;
        return true;
    case REG_SW:
        _sw = value;
        break;
    case REG_LINK:
        set_link(value);
        break;
    case REG_IEFF:
        set_ieff(value);
        break;
    }
    return false;
}

}  // namespace pdp8
}  // namespace debugger

// Local Variables:
// mode: c++
// c-basic-offset: 4
// tab-width: 4
// End:
// vim: set ft=cpp et ts=4 sw=4:
