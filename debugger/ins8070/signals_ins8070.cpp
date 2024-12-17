#include "signals_ins8070.h"
#include "char_buffer.h"
#include "debugger.h"
#include "digital_bus.h"
#include "inst_ins8070.h"
#include "pins_ins8070.h"

namespace debugger {
namespace ins8070 {

bool Signals::fetch() const {
    // check at least 5 bus cycles.
    // this may not work for SSM instruction.
    if (write() || get()->diff(this) < 6)
        return false;
    InstIns8070 inst;
    const auto end = next();
    // needs at least 2 valid bus cycles.
    for (auto i = 2; i < 6; ++i) {
        if (inst.match(prev(i), end))
            return inst.matchedCycles() == i;
    }
    return false;
}

bool Signals::getDirection() {
    cntl() = busRead(CNTL);
    return cntl() != (CNTL_RDS | CNTL_WDS);
}

bool Signals::read() const {
    return (cntl() & CNTL_RDS) == 0;
}

bool Signals::write() const {
    return (cntl() & CNTL_WDS) == 0;
}

void Signals::getAddr() {
    addr = busRead(AL) | busRead(AM) | busRead(AH);
}

void Signals::getData() {
    data = busRead(D);
    markFetch(false);
}

void Signals::outData() const {
    busWrite(D, data);
    busMode(D, OUTPUT);
}

void Signals::inputMode() {
    busMode(D, INPUT);
}

void Signals::print() const {
    LOG_MATCH(cli.printDec(pos(), -4));
    //                              0123456789012
    static constexpr char line[] = "R A=xxxx D=xx";
    static auto &buffer = *new CharBuffer(line);
    buffer[0] = read() ? 'R' : 'W';
    buffer.hex16(4, addr);
    buffer.hex8(11, data);
    cli.println(buffer);
}

}  // namespace ins8070
}  // namespace debugger

// Local Variables:
// mode: c++
// c-basic-offset: 4
// tab-width: 4
// End:
// vim: set ft=cpp et ts=4 sw=4:
