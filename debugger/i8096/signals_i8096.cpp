#include "signals_i8096.h"
#include "char_buffer.h"
#include "debugger.h"
#include "digital_bus.h"
#include "pins_i8096.h"

namespace debugger {
namespace i8096 {

bool SignalsI8096::getAddrValid() const {
    // #ADV is active low
    return digitalReadFast(PIN_ADV) == LOW;
}

void SignalsI8096::getAddr() {
    addr = busRead(AD);
}

void SignalsI8096::getData() {
    data = busRead(AD);
}

void SignalsI8096::outData() const {
    busWrite(AD, data);
    busMode(AD, OUTPUT);
}

void SignalsI8096::outLow() const {
    busWrite(DATA, data);
    busMode(DATA, OUTPUT);
}

void SignalsI8096::inputMode() const {
    busMode(AD, INPUT);
}

uint_fast8_t SignalsI8096::bytes() const {
    if (addr & 1)
        return 1;
    // #BHE is active low
    return read() || (cntl() & CNTL_BHE) == 0 ? 2 : 1;
}

uint8_t SignalsI8096::byteAt(uint16_t a) const {
    // The even byte rides AD0-AD7, the odd one AD8-AD15.
    return (a & 1) ? data >> 8 : data & 0xFF;
}

bool SignalsI8096::fetch() const {
    return (cntl() & (CNTL_ADV | CNTL_FETCH)) == 0;
}

bool SignalsI8096::read() const {
    return (cntl() & (CNTL_ADV | CNTL_RD)) == 0;
}

bool SignalsI8096::write() const {
    return (cntl() & (CNTL_ADV | CNTL_WR)) == 0;
}

void SignalsI8096::clearMark() {
    // CNTL_FETCH is active low
    cntl() |= CNTL_FETCH;
    mark() = 0;
}

void SignalsI8096::markFetch() {
    // CNTL_FETCH is active low
    cntl() &= ~CNTL_FETCH;
}

void SignalsI8096::print() const {
    LOG_MATCH(cli.print(readMemory() ? ' ' : 'i'));
    LOG_MATCH(cli.print(writeMemory() ? ' ' : 'c'));
    LOG_MATCH(cli.print(isOperand() ? 'o' : ' '));
    LOG_MATCH(cli.print(' '));
    LOG_MATCH(cli.printDec(pos(), -4));
    //                              012345678901234
    static constexpr char line[] = "  A=xxxx D=xx";
    static constexpr char word[] = "  A=xxxx D=xxxx";
    auto &buffer = Cycles::buffer();
    const auto two = bytes() == 2;
    buffer.set(two ? word : line);
    if (fetch()) {
        buffer[0] = 'I';
    } else if (read()) {
        buffer[0] = 'R';
    } else if (write()) {
        buffer[0] = 'W';
    }
    buffer.hex16(4, addr);
    if (two) {
        buffer.hex16(11, data);
    } else {
        buffer.hex8(11, byteAt(addr));
    }
    cli.println(buffer);
}

}  // namespace i8096
}  // namespace debugger

// Local Variables:
// mode: c++
// c-basic-offset: 4
// tab-width: 4
// End:
// vim: set ft=cpp et ts=4 sw=4:
