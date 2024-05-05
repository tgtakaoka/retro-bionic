#include "signals_mos6502.h"
#include "char_buffer.h"
#include "debugger.h"
#include "digital_bus.h"
#include "inst_mos6502.h"
#include "pins_mos6502.h"

namespace debugger {
namespace mos6502 {

void Signals::getAddr() {
    cntl() = busRead(CNTL);
    addr = busRead(AL) | busRead(AM) | busRead(AH);
}

bool Signals::read() const {
    return cntl() & CNTL_RW;
}

bool Signals::write() const {
    return (cntl() & CNTL_RW) == 0;
}

bool Signals::fetch() const {
    if (Pins.hardwareType() == HW_W65C816)
        return (~cntl() & (CNTL_VPA | CNTL_VPD)) == 0;
    return cntl() & CNTL_SYNC;
}

bool Signals::vector() const {
    if (Pins.hardwareType() == HW_MOS6502)
        return read() && addr >= InstMos6502::VECTOR_NMI;
    return (cntl() & CNTL_VP) == 0;
}

bool Signals::addr24() const {
    return (cntl() & CNTL_E) == 0;
}

void Signals::getData() {
    data = busRead(D);
}

void Signals::print() const {
    if (Pins.hardwareType() == HW_W65C816) {
        //                              0123456789012345
        static constexpr char line[] = "VW A=xxxxxx D=xx";
        static auto &buffer = *new CharBuffer(line);
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
        //                              0123456789012345
        static constexpr char line[] = "VW A=xxxx D=xx  ";
        static auto &buffer = *new CharBuffer(line);
        buffer.hex16(5, addr);
        buffer.hex8(12, data);
        buffer[0] = fetch() ? 'S' : (vector() ? 'V' : ' ');
        buffer[1] = write() ? 'W' : 'R';
        buffer.hex4(15, cntl());
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
