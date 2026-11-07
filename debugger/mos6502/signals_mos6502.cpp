#include "signals_mos6502.h"
#include "char_buffer.h"
#include "debugger.h"
#include "digital_bus.h"
#include "inst_mos6502.h"
#include "pins_mos6502.h"

namespace debugger {
namespace mos6502 {

uint32_t Signals::_dataInput;
uint32_t Signals::_dataOutput;

void Signals::saveDataMode() {
    _dataInput = PDDR(D) & ~PORT_gm(D);
    _dataOutput = _dataInput | PORT_gm(D);
}

bool Signals::fetch() const {
    if (PinsMos6502::hardwareType() == HW_W65C816)
        return fetch<HW_W65C816>();
    return fetch<HW_MOS6502>();
}

bool Signals::vector() const {
    if (PinsMos6502::hardwareType() == HW_MOS6502)
        return vector<HW_MOS6502>();
    return vector<HW_W65C02S>();
}

void Signals::print() const {
    if (PinsMos6502::hardwareType() == HW_W65C816) {
        //                              0123456789012345
        static constexpr char line[] = "VW A=xxxxxx D=xx";
        auto &buffer = Cycles::buffer();
        buffer.set(line);
        buffer[0] = fetch() ? 'S' : (vector() ? 'V' : ' ');
        buffer[1] = write() ? 'W' : 'R';
        if (addr24()) {
            buffer.hex24(5, addr);
        } else {
            buffer[5] = buffer[6] = ' ';
            buffer.hex16(7, addr);
        }
        buffer.hex8(14, data);
        cli.println(buffer);
    } else {
        //                              01234567890123
        static constexpr char line[] = "VW A=xxxx D=xx";
        auto &buffer = Cycles::buffer();
        buffer.set(line);
        buffer.hex16(5, addr);
        buffer.hex8(12, data);
        buffer[0] = fetch() ? 'S' : (vector() ? 'V' : ' ');
        buffer[1] = write() ? 'W' : 'R';
        cli.println(buffer);
    }
}

}  // namespace mos6502
}  // namespace debugger

// Local Variables:
// mode: c++
// c-basic-offset: 4
// tab-width: 4
// End:
// vim: set ft=cpp et ts=4 sw=4:
