#include "signals_ins8070.h"
#include "char_buffer.h"
#include "debugger.h"
#include "digital_bus.h"
#include "inst_ins8070.h"
#include "pins_ins8070.h"

namespace debugger {
namespace ins8070 {

namespace {
// The bytes the cycles [begin, end) read: what the CPU decoded.
struct BusReads final : MatchMemory {
    BusReads(const Signals *begin, const Signals *end)
        : _begin(begin), _size(begin->diff(end)) {}
    uint16_t read_byte(uint32_t addr) const override {
        for (auto i = _size; i-- > 0;) {
            const auto s = _begin->next(i);
            if (s->read() && s->addr == addr)
                return s->data;
        }
        return 0xFF;
    }

private:
    const Signals *const _begin;
    const uint_fast8_t _size;
};
}  // namespace

// Whether an instruction of 2 to 5 cycles ends right before this one.
bool Signals::fetch() const {
    // check at least 5 bus cycles.
    // this may not work for SSM instruction.
    if (write() || get()->diff(this) < 6)
        return false;
    const auto begin = const_cast<Signals *>(prev(5));
    const auto end = next();
    const BusReads reads(begin, end);
    const ArchIns8070 arch(reads);
    auto &walker = MatchWalker::shared();
    // needs at least 2 valid bus cycles.
    for (auto i = 2u; i < 6; ++i) {
        const auto cycles = walker.matchInstruction(
                arch, const_cast<Signals *>(prev(i)), end);
        if (cycles)
            return cycles == i;
    }
    return false;
}

bool Signals::getDirection() {
    cntl() = busRead(CNTL);
    return cntl() != (CNTL_RDS | CNTL_WDS);
}

void Signals::noCycle() {
    cntl() = CNTL_RDS | CNTL_WDS;
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
    auto &buffer = Cycles::buffer();
    buffer.set(line);
    buffer[0] = read() ? 'R' : 'W';
    buffer.hex16(4, addr);
    buffer.hex8(11, data);
#ifdef PROFILE_CYCLES
    constexpr auto suffix = true;
#else
    const auto suffix = Debugger.verbose();  // a bench recording's
#endif
    if (suffix) {
        // The matcher's mark, for tools/cycles_ins8070.py.
        cli.print(buffer);
        cli.println(fetchMark() ? " L" : "");
    } else {
        cli.println(buffer);
    }
}

}  // namespace ins8070
}  // namespace debugger

// Local Variables:
// mode: c++
// c-basic-offset: 4
// tab-width: 4
// End:
// vim: set ft=cpp et ts=4 sw=4:
