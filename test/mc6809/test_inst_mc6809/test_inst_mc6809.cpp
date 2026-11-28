// Host-side test of the MC6809 family's cycle matcher: `pio test -e native`.
//
// <set>.cycles.zst are the recorded rings (see test/match_rings.py),
// <set>.marks.zst what the matcher made of them (see
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
namespace mc6809 {
bool Signals::syncAck() const {
    return (cntl() & 0xC) == 0x8;  // CNTL_BA, not CNTL_BS
}
}  // namespace mc6809
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

// A set's goldens, then how its marks compare with the recording's.
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

// The rings a halt leaves where the CPU waits: LDS #$0FFF at 0100, then
// CWAI #$FF or SYNC at 0104. The board keeps a wait's first two cycles.
constexpr auto WAITS = 2;

struct WaitRing {
    std::vector<Cycle> cycles;
    void read(uint32_t addr, uint16_t data, int cntl = 3) {  // RW, VMA
        cycles.push_back(Cycle{'R', addr, data, 0, cntl});
    }
    void lds() {
        read(0x0100, 0x10);
        read(0x0101, 0xCE);
        read(0x0102, 0x0F);
        read(0x0103, 0xFF);
    }
    template <SoftwareType TYPE = SW_MC6809>
    int walk(const char *name, std::vector<int> &marks) {
        current = name;
        return replay<TYPE>(cycles, marks);
    }
};

// CWAI pushes, waits, and takes NMI's vector without a push; the ring
// ends at the handler's fetch, the opcode the board injects there. The
// HD6309's native mode reads FFFF where the MC6809 reads ahead, and
// pushes E and F too.
template <SoftwareType TYPE>
void cwaiHalt(const char *name, bool native) {
    WaitRing r;
    r.lds();
    r.read(0x0104, 0x3C);
    r.read(0x0105, 0xFF);
    r.read(native ? 0xFFFF : 0x0106, native ? 0 : 0x20);
    r.read(0xFFFF, 0);
    const uint8_t push[] = {
            0x06, 0x01, 0, 0, 0, 0, 0, 0, 0xFF, 0xFF, 0, 0xFF, 0, 0xD1};
    const auto pushes = native ? 14u : 12u;
    for (auto i = 0u; i < pushes; ++i) {
        const auto skip = !native && (i == 8 || i == 9);  // no E, F
        r.cycles.push_back(Cycle{'W', 0x0FFEu - i, push[i + 2 * skip], 0, 1});
    }
    for (auto i = 0; i < WAITS; ++i)
        r.read(0xFFFF, 0);
    r.read(0xFFFC, 0x30, 7);  // BS
    r.read(0xFFFD, 0x17, 7);
    r.read(0xFFFF, 0);
    r.read(0x3017, 0x3B);
    memory = "3017:12";  // not the injected RTI
    std::vector<int> marks;
    const auto start = r.walk<TYPE>(name, marks);
    memory = nullptr;
    TEST_ASSERT_EQUAL(0, start);
    TEST_ASSERT_NOT_EQUAL(0, marks[0]);  // LDS
    TEST_ASSERT_NOT_EQUAL(0, marks[4]);  // CWAI
}

void test_cwai_halt() {
    cwaiHalt<SW_MC6809>("cwai", false);
    cwaiHalt<SW_HD6309>("cwai_hd6309", false);
    cwaiHalt<SW_HD6309>("cwai_native", true);
}

// SYNC's acknowledge cycles, BA without BS, then the fetch NMI ends it at.
void test_sync_halt() {
    WaitRing r;
    r.lds();
    r.read(0x0104, 0x13);
    r.read(0x0105, 0x20);
    for (auto i = 0; i < WAITS; ++i)
        r.read(0xFF05, 0xF6, 0xB);  // BA
    r.read(0xFF05, 0xF6);
    r.read(0x0105, 0x20);
    std::vector<int> marks;
    const auto start = r.walk("sync", marks);
    TEST_ASSERT_EQUAL(0, start);
    TEST_ASSERT_NOT_EQUAL(0, marks[0]);  // LDS
    TEST_ASSERT_NOT_EQUAL(0, marks[4]);  // SYNC
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
    RUN_TEST(test_cwai_halt);
    RUN_TEST(test_sync_halt);
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
