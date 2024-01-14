#include "signals_mos6502.h"
#include "char_buffer.h"
#include "debugger.h"
#include "digital_bus.h"
#include "inst_mos6502.h"
#include "pins_mos6502.h"

namespace debugger {
namespace mos6502 {

void Signals::getAddr() {
    rw() = digitalReadFast(PIN_RW);
    addr = busRead(AL) | busRead(AM) | busRead(AH);
    if (Pins.hardwareType() == HW_MOS6502) {
        fetchVector() = read() && addr >= InstMos6502::VECTOR_NMI;
    } else {
        fetchVector() = digitalReadFast(PIN_VP) == LOW;
    }
}

void Signals::getData() {
    data = busRead(D);
}

void Signals::print() const {
    if (Pins.native65816()) {
        //                              0123456789012345
        static constexpr char line[] = "VW A=xxxxxx D=xx";
        static auto &buffer = *new CharBuffer(line);
        buffer[0] = fetch() ? 'S' : (vector() ? 'V' : ' ');
        buffer[1] = write() ? 'W' : 'R';
        buffer.hex24(5, addr);
        buffer.hex8(14, data);
        cli.println(buffer);
    } else {
        //                              01234567890123
        static constexpr char line[] = "VW A=xxxx D=xx";
        static auto &buffer = *new CharBuffer(line);
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
