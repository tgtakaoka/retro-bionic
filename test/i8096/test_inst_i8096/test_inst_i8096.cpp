// Host-side test of the i8096 cycle matcher: `pio test -e native`.
//
// <set>.cycles.zst are the recorded rings (see test/match_rings.py),
// <set>.marks.zst what the matcher made of them (see
// test/match_harness.h).

#include "../../match_harness.h"

#define protected public  // the rings are fed straight into _signals
#include "i8096/signals_i8096.h"
#undef protected
#include "i8096/inst_i8096.cpp"

using namespace debugger;
using namespace debugger::i8096;
using namespace match_harness;

namespace {

// The fixtures: next to this file.
const auto DIR = dirOf(__FILE__);
// pins_i8096.h's control lines.
constexpr uint8_t CNTL_ADV = 0x1;
constexpr uint8_t CNTL_RD = 0x2;
constexpr uint8_t CNTL_WR = 0x4;
constexpr uint8_t CNTL_START0 = 0x40;
constexpr uint8_t CNTL_START1 = 0x80;
}  // namespace

namespace debugger {
namespace i8096 {
// As signals_i8096.cpp has them, which needs the board.
bool SignalsI8096::fetch() const {
    return (cntl() & CNTL_ADV) == 0 && (cntl() & (CNTL_START0 | CNTL_START1));
}
bool SignalsI8096::read() const {
    return (cntl() & (CNTL_ADV | CNTL_RD)) == 0;
}
bool SignalsI8096::inst() const {
    return false;
}
bool SignalsI8096::write() const {
    return (cntl() & (CNTL_ADV | CNTL_WR)) == 0;
}
// What the recording printed: a byte or a word a cycle, the bytes in
// _signals[2]; none on an 8-bit bus, a byte a cycle.
uint_fast8_t SignalsI8096::bytes() const {
    return _signals[2] ? _signals[2] : 1;
}
uint8_t SignalsI8096::byteAt(uint16_t a) const {
    return bytes() == 2 && (a & 1) ? data >> 8 : data & 0xFF;
}
void SignalsI8096::clearMark() {
    cntl() &= ~(CNTL_START0 | CNTL_START1);
    mark() = 0;
}
void SignalsI8096::markStart(uint16_t pc) {
    if (pc == addr) {
        cntl() |= CNTL_START0;
    } else if (pc == static_cast<uint16_t>(addr + 1)) {
        cntl() |= CNTL_START1;
    }
}
}  // namespace i8096
}  // namespace debugger

namespace {

using Signals = SignalsI8096;

const auto RINGS = loadRings(DIR, "i8096");
// The profile's runs on the 16-bit bus.
const auto RINGS16 = loadRings(DIR, "i8096_16");

// The bench rings: where running samples stopped (scripts/record-rings.py),
// on an 8-bit bus and on a 16-bit one.
const auto BENCH = loadRings(DIR, "bench_i8096");
const auto BENCH16 = loadRings(DIR, "bench_i8096_16");

// A raw mark: 1 for fetch(), 2 for an operand.
int markOf(const Signals *s) {
    return (s->fetch() ? 1 : 0) | (s->isOperand() ? 2 : 0);
}

// What the profile image prints: I for fetch().
int recorded(int mark) {
    return mark & 1;
}

int replay(const std::vector<Cycle> &cycles, std::vector<int> &marks) {
    Cycles ring;
    for (const auto &c : cycles) {
        const auto s = Signals::put();
        s->addr = c.addr;
        s->data = c.data;
        s->cntl() = c.kind == 'W' ? CNTL_RD : CNTL_WR;
        s->mark() = 0;
        s->_signals[2] = c.cntl;
        Cycles::next();
    }
    const RingMemory memory(cycles, InstI8096::TRAP);
    const auto begin = Signals::get();
    const auto end = Signals::put();
    const ArchI8096 arch(&memory);
    static MatchWalker walker;
    MatchWalker::trace = tracing();
    const auto start =
            walker.walk(arch, begin, end) ? begin->next(walker.start()) : end;
    for (auto i = 0u; i < cycles.size(); ++i)
        marks.push_back(markOf(begin->next(i)));
    return start == end ? -1 : begin->diff(start);
}

}  // namespace

void setUp() {}

void tearDown() {}

void test_recorded_runs() {
    check(DIR, "i8096", RINGS, replay);
}

void test_recorded_runs16() {
    check(DIR, "i8096_16", RINGS16, replay);
}

void test_bench_rings() {
    // the fetch bit; the operand one is the matcher's own
    match_harness::isFetch = [](int mark) { return (mark & 1) != 0; };
    check(DIR, "bench_i8096", BENCH, replay);
}

void test_bench16_rings() {
    match_harness::isFetch = [](int mark) { return (mark & 1) != 0; };
    match_harness::fetchWidth = [](const Cycle &c) {
        return c.cntl ? unsigned(c.cntl) : 1u;
    };
    check(DIR, "bench_i8096_16", BENCH16, replay);
}

void test_report_recorded_marks() {
    report(RINGS, replay, recorded, true);
    report(RINGS16, replay, recorded, true);
}

// Every sequence it walks with is the legend's.
void test_sequences() {
    checkSequences(SEQUENCES);
    checkInterrupt(INTERRUPT);
}

int main() {
    UNITY_BEGIN();
    RUN_TEST(test_sequences);
    RUN_TEST(test_recorded_runs);
    RUN_TEST(test_recorded_runs16);
    RUN_TEST(test_bench_rings);
    RUN_TEST(test_bench16_rings);
    RUN_TEST(test_report_recorded_marks);
    return UNITY_END();
}

// Local Variables:
// mode: c++
// c-basic-offset: 4
// tab-width: 4
// End:
// vim: set ft=cpp et ts=4 sw=4:
