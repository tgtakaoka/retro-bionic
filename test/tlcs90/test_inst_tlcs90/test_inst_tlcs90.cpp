// Host-side test of the TLCS-90 cycle matcher: `pio test -e native`.
//
// <set>.cycles.zst are the recorded rings (see test/match_rings.py),
// <set>.marks.zst what the matcher made of them (see
// test/match_harness.h).

#include "../../match_harness.h"

#define protected public  // the rings are fed straight into _signals
#define private public
#include "tlcs90/signals_tlcs90.h"
#undef private
#undef protected
#include "tlcs90/inst_tlcs90.cpp"

using namespace debugger;
using namespace debugger::tlcs90;
using namespace match_harness;

namespace debugger {
namespace tlcs90 {
// The real ones need the board's pin assignment; the kind is kept as is.
bool Signals::read() const {
    return cntl() == 'R';
}
bool Signals::write() const {
    return cntl() == 'W';
}
}  // namespace tlcs90
}  // namespace debugger

namespace {

// The fixtures: next to this file.
const auto DIR = dirOf(__FILE__);

const auto RINGS = loadRings(DIR, "tlcs90");

// The bench rings: where running samples stopped (scripts/record-rings.py).
const auto BENCH = loadRings(DIR, "bench_tlcs90");

// What the recordings filled memory with.
constexpr uint8_t FILL = InstTlcs90::SWI;

// What the profile image prints: the cycles matched from a fetch.
int recorded(int mark) {
    return mark ? mark - 1 : 0;
}

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
    const auto begin = Signals::get();
    const auto end = Signals::put();
    const RingMemory memory(cycles, FILL);
    const InstTlcs90 inst(&memory);
    static MatchWalker walker;
    MatchWalker::trace = tracing();
    const auto start =
            walker.walk(inst, begin, end) ? begin->next(walker.start()) : end;
    for (auto i = 0u; i < cycles.size(); ++i)
        marks.push_back(begin->next(i)->_signals[1]);
    return start == end ? -1 : begin->diff(start);
}

}  // namespace

void setUp() {}

void tearDown() {}

void test_recorded_runs() {
    check(DIR, "tlcs90", RINGS, replay);
}

void test_bench_rings() {
    check(DIR, "bench_tlcs90", BENCH, replay);
}

void test_report_recorded_marks() {
    report(RINGS, replay, recorded);
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
