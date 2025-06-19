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
    data = busRead(DATA);
}

void SignalsI8096::outData() const {
    busWrite(DATA, data);
    busMode(DATA, OUTPUT);
}

void SignalsI8096::inputMode() const {
    busMode(DATA, INPUT);
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
    //                              0123456789012
    static constexpr char line[] = "W A=xxxx D=xx";
    static auto &buffer = *new CharBuffer(line);
    if (fetch()) {
        buffer[0] = 'I';
    } else if (read()) {
        buffer[0] = 'R';
    } else if (write()) {
        buffer[0] = 'W';
    }
    buffer.hex16(4, addr);
    buffer.hex8(11, data);
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
