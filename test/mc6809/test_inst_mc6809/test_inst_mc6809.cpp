// Host-side test of the MC6809 family's cycle matcher: `pio test -e native`.
//
// rings_<chip>.inc are the bench recordings (see test/match_rings.py);
// golden_<chip>.inc what the matcher made of them (see
// test/match_harness.h). Every board matches with the HD6309's tables.

#include "../../match_harness.h"

#define protected public  // the rings are fed straight into _signals
#include "mc6809/signals_mc6809.h"
#undef protected
#include "mc6809/inst_mc6809.cpp"
#include "mc6809/inst_hd6309.cpp"

using namespace debugger;
using namespace debugger::mc6809;
using namespace match_harness;

namespace debugger {
namespace mc6800 {
// The real ones read the board's pins into the same CNTL bits.
bool Signals::read() const {
    return cntl() & 2;  // CNTL_RW
}
bool Signals::write() const {
    return (cntl() & 2) == 0;
}
bool Signals::valid() const {
    return cntl() & 1;  // CNTL_VMA
}
}  // namespace mc6800
}  // namespace debugger

namespace {

// The fixtures: next to this file.
const auto DIR = dirOf(__FILE__);

const auto RINGS_MC6809 = loadRings(DIR, "mc6809");
const auto RINGS_MC6809E = loadRings(DIR, "mc6809e");
const auto RINGS_HD6309 = loadRings(DIR, "hd6309");
const auto RINGS_HD6309E = loadRings(DIR, "hd6309e");

// The bench rings: where running samples stopped (scripts/record-rings.py).
const auto BENCH_MC6809 = loadRings(DIR, "bench_mc6809");

const auto BENCH_HD6309 = loadRings(DIR, "bench_hd6309");

const auto BENCH_MC6809E = loadRings(DIR, "bench_mc6809e");

const auto BENCH_HD6309E = loadRings(DIR, "bench_hd6309e");

// The MC6809 samples on an HD6309, in its emulation mode.
const auto BENCH_HD6309_MC6809 = loadRings(DIR, "bench_hd6309_mc6809");

// The MC6809 samples on an HD6309E, in its emulation mode.
const auto BENCH_HD6309E_MC6809 = loadRings(DIR, "bench_hd6309e_mc6809");

// What the recordings filled memory with: SWI.
constexpr uint8_t FILL = 0x3F;

// What the profile image prints: the cycles matched from a fetch.
int recorded(int mark) {
    return mark ? mark - 1 : 0;
}

template <SoftwareType TYPE>
int replay(const std::vector<Cycle> &cycles, std::vector<int> &marks) {
    Cycles ring;
    for (const auto &c : cycles) {
        const auto s = Signals::put();
        s->addr = c.addr;
        s->data = c.data;
        s->cntl() = c.cntl;
        s->_signals[1] = 0;
        Cycles::next();
    }
    const RingMemory memory(cycles, FILL);
    hd6309::InstHd6309 inst(&memory);
    inst.setSoftwareType(TYPE);
    const auto begin = Signals::get();
    const auto end = Signals::put();
    static MatchWalker walker;
    MatchWalker::trace = tracing();
    const auto start =
            walker.walk(inst, begin, end) ? begin->next(walker.start()) : end;
    for (auto i = 0u; i < cycles.size(); ++i)
        marks.push_back(begin->next(i)->_signals[1]);
    return start == end ? -1 : begin->diff(start);
}

template <SoftwareType TYPE>
void check(const char *set, const std::vector<Ring> &rings) {
    match_harness::check(DIR, set, rings, replay<TYPE>);
    report(rings, replay<TYPE>, recorded);
}

}  // namespace

void setUp() {}

void tearDown() {}

void test_mc6809() {
    check<SW_MC6809>("mc6809", RINGS_MC6809);
}

void test_bench_mc6809() {
    match_harness::check(DIR, "bench_mc6809", BENCH_MC6809, replay<SW_MC6809>);
}

void test_mc6809e() {
    check<SW_MC6809>("mc6809e", RINGS_MC6809E);
}

void test_bench_mc6809e() {
    match_harness::check(DIR, "bench_mc6809e", BENCH_MC6809E, replay<SW_MC6809>);
}

void test_hd6309() {
    check<SW_HD6309>("hd6309", RINGS_HD6309);
}

void test_bench_hd6309() {
    match_harness::check(DIR, "bench_hd6309", BENCH_HD6309, replay<SW_HD6309>);
}

void test_hd6309e() {
    check<SW_HD6309>("hd6309e", RINGS_HD6309E);
}

void test_bench_hd6309e() {
    match_harness::check(DIR, "bench_hd6309e", BENCH_HD6309E, replay<SW_HD6309>);
}

void test_bench_hd6309_mc6809() {
    match_harness::check(DIR, "bench_hd6309_mc6809", BENCH_HD6309_MC6809, replay<SW_HD6309>);
}

void test_bench_hd6309e_mc6809() {
    match_harness::check(DIR, "bench_hd6309e_mc6809", BENCH_HD6309E_MC6809, replay<SW_HD6309>);
}

// Every sequence it walks with is the legend's, but the postbyte's,
// pulls' and pushes' its decoder expands.
void test_sequences() {
    checkSequences(hd6309::SEQUENCES, "#<>");
    checkInterrupt(hd6309::INTERRUPT_6809);
    checkInterrupt(hd6309::INTERRUPT_6309);
}

int main() {
    UNITY_BEGIN();
    RUN_TEST(test_sequences);
    RUN_TEST(test_mc6809);
    RUN_TEST(test_bench_mc6809);
    RUN_TEST(test_mc6809e);
    RUN_TEST(test_bench_mc6809e);
    RUN_TEST(test_hd6309);
    RUN_TEST(test_bench_hd6309);
    RUN_TEST(test_hd6309e);
    RUN_TEST(test_bench_hd6309e);
    RUN_TEST(test_bench_hd6309_mc6809);
    RUN_TEST(test_bench_hd6309e_mc6809);
    return UNITY_END();
}

// Local Variables:
// mode: c++
// c-basic-offset: 4
// tab-width: 4
// End:
// vim: set ft=cpp et ts=4 sw=4:
