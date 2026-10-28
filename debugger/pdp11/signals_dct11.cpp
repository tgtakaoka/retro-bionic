#include "signals_dct11.h"
#include "char_buffer.h"
#include "debugger.h"
#include "digital_bus.h"
#include "pins_dct11.h"

namespace debugger {
namespace dct11 {

namespace {
// Three octal digits of a byte.
void oct8(CharBuffer &buffer, uint_fast8_t pos, uint8_t val) {
    buffer.oct3(pos, val >> 6);
    buffer.oct6(pos + 1, val);
}
}  // namespace

void Signals::getAddr() {
    addr = busRead(DAL);
}

void Signals::getControl() {
    const uint32_t gpio7 = PIN(CNTL);
    _signals[0] = busValue(CNTL, gpio7);
    _signals[1] = busValue(SEL, gpio7);
}

void Signals::getData() {
    data = busRead(DAL);
}

void Signals::outData() const {
    busWriteFast(DAL, data);
    busMode(DAL, OUTPUT);
}

void Signals::outVector() const {
    busWriteFast(DALL, data);
    busMode(DALL, OUTPUT);
}

void Signals::inputMode() const {
    busMode(DAL, INPUT);
}

void Signals::print() const {
#ifdef PROFILE_CYCLES
    // slot, and inject/capture flags
    cli.print(readMemory() ? ' ' : 'i');
    cli.print(writeMemory() ? ' ' : 'c');
    cli.print(' ');
    cli.printDec(pos(), -4);
#endif
    //                              01234567890123456
    static constexpr char line[] = "R A=xxxxxx D=xxxxxx";
    auto &buffer = Cycles::buffer();
    buffer.set(line);
    if (iack()) {
        buffer[0] = 'V';
    } else if (fetch()) {
        buffer[0] = 'I';
    } else if (write()) {
        buffer[0] = 'W';
    }
    buffer.oct18(4, addr);
    if (byteWrite()) {
        // The byte on the half its R/#Wxx line picks.
        buffer[13] = buffer[14] = buffer[15] = ' ';
        oct8(buffer, 16, writeLow() ? data : (data >> 8));
    } else {
        buffer.oct18(13, data);
    }
    cli.println(buffer);
}

}  // namespace dct11
}  // namespace debugger

// Local Variables:
// mode: c++
// c-basic-offset: 4
// tab-width: 4
// End:
// vim: set ft=cpp et ts=4 sw=4:
