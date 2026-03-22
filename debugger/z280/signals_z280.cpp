#include "signals_z280.h"
#include "char_buffer.h"
#include "debugger.h"
#include "digital_bus.h"
#include "pins_z280.h"

namespace debugger {
namespace z280 {

void Signals::getAddr() {
    addr = busRead(AD) | busRead(AL) | busRead(AH);
}

void Signals::getControl() {
    st() = busRead(ST);
    rw() = digitalReadFast(PIN_RW);
    bw() = digitalReadFast(PIN_BW);
}

void Signals::getData() {
    data = busRead(AD);
}

void Signals::outData() const {
    busWrite(AD, data);
    busMode(AD, OUTPUT);
}

void Signals::inputMode() const {
    busMode(AD, INPUT);
}

void Signals::print() const {
    // cli.print(readMemory() ? ' ' : 'i');
    // cli.print(writeMemory() ? ' ' : 'c');
    // cli.print(' ');
    // cli.printDec(pos(), -4);
    //                              01234567890123456
    static constexpr char line[] = "R A=xxxxxx D=xxxx";
    static auto &buffer = *new CharBuffer(line);
    buffer[2] = (ioReq()) ? 'I' : 'A';
    /* if (fetch()) {
        buffer[0] = 'I';
        } else */
    if (read()) {
        buffer[0] = 'R';
    } else if (write()) {
        buffer[0] = 'W';
    }
    buffer.hex24(4, addr);
    if (wordAccess()) {
        buffer.hex16(13, data);
    } else if (addr & 1) {
        buffer.hex8(13, data >> 8);
        buffer[15] = buffer[16] = ' ';
    } else {        
        buffer[13] = buffer[14] = ' ';
        buffer.hex8(15, data);
    }
    cli.println(buffer);
}

}  // namespace z280
}  // namespace debugger

// Local Variables:
// mode: c++
// c-basic-offset: 4
// tab-width: 4
// End:
// vim: set ft=cpp et ts=4 sw=4:
