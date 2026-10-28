#include "signals_cp1610.h"
#include "char_buffer.h"
#include "debugger.h"
#include "digital_bus.h"
#include "pins_cp1610.h"

namespace debugger {
namespace cp1600 {

void Signals::getControl() {
    cntl() = busRead(CNTL);
}

void Signals::getAddr() {
    addr = busRead(DATA);
}

void Signals::getData() {
    data = busRead(DATA);
}

void Signals::outData() const {
    busWriteFast(DATA, data);
    busMode(DATA, OUTPUT);
}

void Signals::inputMode() {
    busMode(DATA, INPUT);
}

bool Signals::read() const {
    const auto b = bus();
    return b == ADAR || b == IAB || b == DTB;
}

bool Signals::write() const {
    return bus() == DWS;
}

bool Signals::fetch() const {
    return (cntl() & CNTL_FETCH) != 0;
}

void Signals::markFetch() {
    cntl() |= CNTL_FETCH;
}

void Signals::clearFetch() {
    cntl() &= ~CNTL_FETCH;
}

void Signals::print() const {
    // cli.printDec(pos(), -4);
    //                              01234567890123456
    static constexpr char line[] = "DTB A=xxxx D=xxxx";
    static constexpr char NAMES[8][4] = {
            "NAC", "BAR", "ADR", "DW ", "IAB", "DWS", "DTB", "INT"};
    auto &buffer = Cycles::buffer();
    buffer.set(line);
    const auto name = fetch() ? "FET" : NAMES[bus()];
    for (auto i = 0; i < 3; ++i)
        buffer[i] = name[i];
    buffer.hex16(6, addr);
    buffer.hex16(13, data);
    cli.println(buffer);
}

}  // namespace cp1600
}  // namespace debugger

// Local Variables:
// mode: c++
// c-basic-offset: 4
// tab-width: 4
// End:
// vim: set ft=cpp et ts=4 sw=4:
