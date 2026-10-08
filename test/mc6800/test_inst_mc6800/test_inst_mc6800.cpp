// Host-side test of the MC6800 family's cycle matcher: `pio test -e native`.
//
// rings_<chip>.inc are the bench recordings (see test/match_rings.py);
// golden_<chip>.inc what the matcher made of them (see
// test/match_harness.h). The MC6800, MC6802 and MB8870 boards match with
// the MB8861's tables.

#include "../../match_harness.h"

#define protected public  // the rings are fed straight into _signals
#include "mc6800/signals_mc6800.h"
#undef protected
#include "mc6800/inst_hd6301.cpp"
#include "mc6800/inst_mb8861.cpp"
#include "mc6800/inst_mc6800.cpp"
#include "mc6800/inst_mc6801.cpp"

using namespace debugger;
using namespace debugger::mc6800;
using namespace match_harness;

namespace debugger {
namespace mc6800 {
// The real ones need the board's pin assignment; the kind is kept as is,
// blank for a cycle with no valid address.
bool Signals::read() const {
    return cntl() != 'W';
}
bool Signals::write() const {
    return cntl() == 'W';
}
bool Signals::valid() const {
    return cntl() != ' ';
}
}  // namespace mc6800
}  // namespace debugger

namespace {

// The fixtures: next to this file.
const auto DIR = dirOf(__FILE__);

const auto RINGS_MC6800 = loadRings(DIR, "mc6800");
const auto RINGS_MC6802 = loadRings(DIR, "mc6802");
const auto RINGS_MB8861 = loadRings(DIR, "mb8861");
const auto RINGS_MB8870 = loadRings(DIR, "mb8870");
const auto RINGS_MC6801 = loadRings(DIR, "mc6801");
const auto RINGS_HD6301 = loadRings(DIR, "hd6301");

// The bench rings: where running samples stopped (scripts/record-rings.py).
const auto BENCH_MC6800 = loadRings(DIR, "bench_mc6800");

const auto BENCH_HD6301 = loadRings(DIR, "bench_hd6301");

const auto BENCH_MC6801 = loadRings(DIR, "bench_mc6801");

const auto BENCH_MB8861 = loadRings(DIR, "bench_mb8861");

const auto BENCH_MC6802 = loadRings(DIR, "bench_mc6802");

const auto BENCH_MB8870 = loadRings(DIR, "bench_mb8870");

// What the recordings filled memory with: SWI.
constexpr uint8_t FILL = ArchMc6800::SWI;

// What the profile image prints: the cycles matched from a fetch.
int recorded(int mark) {
    return mark ? mark - 1 : 0;
}

template <typename INST>
int replay(const std::vector<Cycle> &cycles, std::vector<int> &marks) {
    Cycles ring;
    for (const auto &c : cycles) {
        const auto s = Signals::put();
        s->addr = c.addr;
        s->data = c.data;
        s->cntl() = c.kind;
        s->_signals[1] = 0;
        Cycles::next();
    }
    const RingMemory memory(cycles, FILL);
    const INST inst(&memory);
    static MatchWalker walker;
    MatchWalker::trace = tracing();
    const auto begin = Signals::get();
    const auto end = Signals::put();
    const auto start =
            walker.walk(inst, begin, end) ? begin->next(walker.start()) : end;
    for (auto i = 0u; i < cycles.size(); ++i)
        marks.push_back(begin->next(i)->_signals[1]);
    return start == end ? -1 : begin->diff(start);
}

template <typename INST>
void check(const char *set, const std::vector<Ring> &rings) {
    match_harness::check(DIR, set, rings, replay<INST>);
    report(rings, replay<INST>, recorded);
}

}  // namespace

void setUp() {}

void tearDown() {}

void test_mc6800() {
    check<mb8861::InstMb8861>("mc6800", RINGS_MC6800);
}

void test_bench_mc6800() {
    match_harness::check(DIR, "bench_mc6800", BENCH_MC6800, replay<mb8861::InstMb8861>);
}

void test_mc6802() {
    check<mb8861::InstMb8861>("mc6802", RINGS_MC6802);
}

void test_bench_mc6802() {
    match_harness::check(DIR, "bench_mc6802", BENCH_MC6802, replay<mb8861::InstMb8861>);
}

void test_mb8861() {
    check<mb8861::InstMb8861>("mb8861", RINGS_MB8861);
}

void test_bench_mb8861() {
    match_harness::check(DIR, "bench_mb8861", BENCH_MB8861, replay<mb8861::InstMb8861>);
}

void test_mb8870() {
    check<mb8861::InstMb8861>("mb8870", RINGS_MB8870);
}

void test_bench_mb8870() {
    match_harness::check(DIR, "bench_mb8870", BENCH_MB8870, replay<mb8861::InstMb8861>);
}

void test_mc6801() {
    check<mc6801::InstMc6801>("mc6801", RINGS_MC6801);
}

void test_bench_mc6801() {
    match_harness::check(DIR, "bench_mc6801", BENCH_MC6801, replay<mc6801::InstMc6801>);
}

void test_hd6301() {
    check<hd6301::InstHd6301>("hd6301", RINGS_HD6301);
}

void test_bench_hd6301() {
    match_harness::check(DIR, "bench_hd6301", BENCH_HD6301, replay<hd6301::InstHd6301>);
}

// Every sequence they walk with is the legend's.
void test_sequences() {
    checkSequences(mb8861::SEQUENCES);
    checkInterrupt(mb8861::INTERRUPT);
    checkSequences(mc6801::SEQUENCES);
    checkInterrupt(mc6801::INTERRUPT);
    checkSequences(hd6301::SEQUENCES);
    checkInterrupt(hd6301::INTERRUPT);
}

int main() {
    UNITY_BEGIN();
    RUN_TEST(test_sequences);
    RUN_TEST(test_mc6800);
    RUN_TEST(test_bench_mc6800);
    RUN_TEST(test_mc6802);
    RUN_TEST(test_bench_mc6802);
    RUN_TEST(test_mb8861);
    RUN_TEST(test_bench_mb8861);
    RUN_TEST(test_mb8870);
    RUN_TEST(test_bench_mb8870);
    RUN_TEST(test_mc6801);
    RUN_TEST(test_bench_mc6801);
    RUN_TEST(test_hd6301);
    RUN_TEST(test_bench_hd6301);
    return UNITY_END();
}

// Local Variables:
// mode: c++
// c-basic-offset: 4
// tab-width: 4
// End:
// vim: set ft=cpp et ts=4 sw=4:
