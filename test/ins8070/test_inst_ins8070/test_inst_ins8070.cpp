// Host-side test of the INS8070 cycle matcher: `pio test -e native`.
//
// <set>.cycles.zst are the recorded rings (see test/match_rings.py),
// <set>.marks.zst what the matcher made of them (see
// test/match_harness.h).

#include "../../match_harness.h"

#define protected public  // the rings are fed straight into _signals
#define private public
#include "ins8070/signals_ins8070.h"
#undef private
#undef protected
#include "ins8070/inst_ins8070.cpp"

using namespace debugger;
using namespace debugger::ins8070;
using namespace match_harness;

namespace debugger {
namespace ins8070 {
// The real ones need the board's pin assignment; the kind is kept as is.
bool Signals::read() const {
    return cntl() == 'R';
}
bool Signals::write() const {
    return cntl() == 'W';
}
}  // namespace ins8070
}  // namespace debugger

namespace {

// The fixtures: next to this file.
const auto DIR = dirOf(__FILE__);

const auto RINGS = loadRings(DIR, "ins8070");

// The bench rings: where running samples stopped (scripts/record-rings.py).
const auto BENCH = loadRings(DIR, "bench_ins8070");

// What the profile image prints: whether a cycle is marked.
int recorded(int mark) {
    return mark;
}

// The walk over a recorded run, as PinsIns8070::findFetch() makes it.
int replay(const std::vector<Cycle> &cycles, std::vector<int> &marks,
        uint32_t stop) {
    Cycles ring;
    for (const auto &c : cycles) {
        const auto s = Signals::put();
        s->addr = c.addr;
        s->data = c.data;
        s->cntl() = c.kind;
        s->_signals[1] = 0;
        Cycles::next();
    }
    // The walk takes in the next fetch past the ring too, which the board
    // leaves there: CALL 15 right after a run's instruction, or a bench
    // ring's breakpoint at its stop.
    InstIns8070 inst;
    const auto peek = Signals::put();
    peek->cntl() = 'R';
    if (match_harness::memory) {
        peek->addr = stop;
    } else {
        peek->addr =
                cycles[0].addr + (inst.get(cycles[0].data) ? inst.len() : 1);
    }
    peek->data = InstIns8070::CALL15;
    auto withPeek = cycles;
    withPeek.push_back(Cycle{'R', peek->addr, peek->data, 0});
    const RingMemory memory(withPeek, 0xFF);
    const ArchIns8070 arch(memory);
    static MatchWalker walker;
    MatchWalker::trace = tracing();
    const auto begin = Signals::get();
    const auto walked = walker.walk(arch, begin, peek->next());
    for (auto i = 0u; i < cycles.size(); ++i)
        marks.push_back(begin->next(i)->_signals[1]);
    return walked ? walker.start() : -1;
}

}  // namespace

void setUp() {}

void tearDown() {}

void test_recorded_runs() {
    check(DIR, "ins8070", RINGS, replay);
}

void test_bench_rings() {
    check(DIR, "bench_ins8070", BENCH, replay);
}

void test_report_recorded_marks() {
    report(RINGS, replay, recorded);
}

// Every sequence it walks with is the legend's.
void test_sequences() {
    checkSequences(BUS_SEQUENCES);
    const RingMemory memory({}, 0xFF);
    checkInterrupt(ArchIns8070(memory).interruptSequence());
}

int main() {
    UNITY_BEGIN();
    RUN_TEST(test_sequences);
    RUN_TEST(test_recorded_runs);
    RUN_TEST(test_bench_rings);
    RUN_TEST(test_report_recorded_marks);
    return UNITY_END();
}

// Local Variables:
// mode: c++
// c-basic-offset: 4
// tab-width: 4
// End:
// vim: set ft=cpp et ts=4 sw=4:
