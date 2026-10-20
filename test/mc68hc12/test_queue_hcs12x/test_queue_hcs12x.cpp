// Host-side test of the HCS12X queue replay: `pio test -e native`.
//
// Each cycle's IQSTAT reports the start of the cycle START_LAG before it
// and the ALD of the one ALD_LAG before it (S12XCPUV2 8.3.2). The cycles
// here are the HCS12 test's, with the status moved to where the HCS12X
// shows it.

#include <unity.h>

#include "mc68hc12/queue_hcs12.cpp"
#include "mc68hc12/queue_hcs12x.cpp"
#include "mc68hc12/queue_mc68hc12.cpp"

using namespace debugger;
using namespace debugger::mc68hc12;

namespace {

Cycles *ring = nullptr;

using Q = QueueHcs12x;

constexpr uint8_t NONE = 0;
constexpr bool ALD = true;
constexpr uint8_t INT = Q::IQ_INT;
constexpr uint8_t SEV = Q::IQ_SEV;
constexpr uint8_t SOD = Q::IQ_SOD;

// The cycles put so far, and the status each one will sample.
uint_fast8_t count = 0;
uint8_t iqstat[32];

Signals *at(uint_fast8_t i) {
    return Signals::get()->next(i);
}

// Adds |iq| to the status of the cycle |lag| after cycle |i|.
void report(uint_fast8_t i, uint8_t lag, uint8_t iq) {
    iqstat[i + lag] |= iq;
}

// A read whose instruction starts with |start| and whose word an ALD takes
// when |ald|.
void fetch(uint16_t addr, uint16_t data, uint8_t start, bool ald) {
    Signals::put()->set(addr, data, true, 0, 0);
    Cycles::next();
    const auto i = count++;
    if (start)
        report(i, Q::START_LAG, start);
    if (ald)
        report(i, Q::ALD_LAG, Q::IQ_ALD);
}

void read(uint16_t addr, uint8_t start = NONE) {
    fetch(addr, 0, start, false);
}

void write(uint16_t addr, uint8_t start = NONE, bool ald = false) {
    Signals::put()->set(addr, 0, false, 0, 0, false);
    Cycles::next();
    const auto i = count++;
    if (start)
        report(i, Q::START_LAG, start);
    if (ald)
        report(i, Q::ALD_LAG, Q::IQ_ALD);
}

// Ends a sequence with the cycles that report the last ones' status.
void replay() {
    for (auto n = 0; n < Q::ALD_LAG; ++n)
        read(0x2000);
    for (uint_fast8_t i = 0; i < count; ++i)
        at(i)->setIqstat(iqstat[i]);
    const auto g = Signals::get();
    Q().replay(g, g->diff(Signals::put()));
}

void resetSequence(uint8_t start, uint16_t org = 0x1000) {
    read(0xFFFE);                       // 0 V
    read(0xFFFE);                       // 1 f
    fetch(org, 0xA7A7, NONE, ALD);      // 2 P
    fetch(org + 2, 0xA7A7, NONE, ALD);  // 3 P
    fetch(org + 4, 0xA7A7, NONE, ALD);  // 4 P
    read(0x2000, start);                // 5 the first instruction
}

}  // namespace

void setUp() {
    ring = new Cycles();
    count = 0;
    for (auto &iq : iqstat)
        iq = 0;
}

void tearDown() {
    delete ring;
    ring = nullptr;
}

void test_reset_even_start() {
    resetSequence(SEV);
    replay();
    TEST_ASSERT_TRUE(at(5)->fetch());
    TEST_ASSERT_EQUAL_HEX16(0x1000, at(5)->inst());
    for (uint_fast8_t i = 0; i < 5; ++i)
        TEST_ASSERT_FALSE(at(i)->fetch());
    TEST_ASSERT_TRUE(at(2)->program());
    TEST_ASSERT_TRUE(at(3)->program());
    TEST_ASSERT_TRUE(at(4)->program());
    TEST_ASSERT_FALSE(at(5)->program());
}

void test_reset_odd_start() {
    resetSequence(SOD);
    replay();
    TEST_ASSERT_TRUE(at(5)->fetch());
    TEST_ASSERT_EQUAL_HEX16(0x1001, at(5)->inst());
}

// One-byte NOPs: a start each cycle, the queue refilled every other one.
void test_nop_run() {
    resetSequence(SEV);
    fetch(0x1006, 0xA7A7, SOD, ALD);  // 6: 1001, takes 1006
    read(0x2000, SEV);                // 7: 1002
    fetch(0x1008, 0xA7A7, SOD, ALD);  // 8: 1003, takes 1008
    read(0x2000, SEV);                // 9: 1004
    replay();
    TEST_ASSERT_EQUAL_HEX16(0x1001, at(6)->inst());
    TEST_ASSERT_EQUAL_HEX16(0x1002, at(7)->inst());
    TEST_ASSERT_EQUAL_HEX16(0x1003, at(8)->inst());
    TEST_ASSERT_EQUAL_HEX16(0x1004, at(9)->inst());
    TEST_ASSERT_TRUE(at(6)->program());
    TEST_ASSERT_FALSE(at(7)->program());
}

// A write is never a program word, whatever the status says.
void test_write_not_program() {
    resetSequence(SEV);
    write(0x0800, NONE, ALD);  // 6
    read(0x2000, SOD);         // 7: still 1001, the queue didn't move
    replay();
    TEST_ASSERT_FALSE(at(6)->program());
    TEST_ASSERT_EQUAL_HEX16(0x1001, at(7)->inst());
}

void test_interrupt_start() {
    resetSequence(SEV);
    read(0xFFF4, INT);  // 6: the vector of an interrupt sequence
    replay();
    TEST_ASSERT_TRUE(at(6)->interrupt());
    TEST_ASSERT_FALSE(at(6)->fetch());
}

// The page 2 prebyte counts as an instruction of its own; the start at its
// opcode folds into it.
void test_page2_second_start() {
    read(0xFFFE);                      // 0 V
    read(0xFFFE);                      // 1 f
    fetch(0x1000, 0x1815, NONE, ALD);  // 2 P: IDIVS
    fetch(0x1002, 0xA7A7, NONE, ALD);  // 3 P
    fetch(0x1004, 0xA7A7, NONE, ALD);  // 4 P
    read(0x2000, SEV);                 // 5: $18 at 1000
    fetch(0x1006, 0xA7A7, SOD, ALD);   // 6: its opcode at 1001
    read(0x2000, SEV);                 // 7: NOP at 1002
    replay();
    TEST_ASSERT_TRUE(at(5)->fetch());
    TEST_ASSERT_EQUAL_HEX16(0x1000, at(5)->inst());
    TEST_ASSERT_FALSE(at(6)->fetch());
    TEST_ASSERT_TRUE(at(7)->fetch());
    TEST_ASSERT_EQUAL_HEX16(0x1002, at(7)->inst());
}

// A combined code: the ALD of one cycle and the start of the next come in
// the same sample.
void test_combined_code() {
    resetSequence(SEV);
    fetch(0x1006, 0xA7A7, SOD, ALD);  // 6
    read(0x2000, SEV);                // 7
    replay();
    // Cycle 8 sampled ALD (of 6) and SEV (of 7) at once.
    TEST_ASSERT_EQUAL_HEX8(Q::IQ_ALD | Q::IQ_SEV, at(8)->iqstat());
    TEST_ASSERT_EQUAL_HEX16(0x1002, at(7)->inst());
}

int main(int, char **) {
    UNITY_BEGIN();
    RUN_TEST(test_reset_even_start);
    RUN_TEST(test_reset_odd_start);
    RUN_TEST(test_nop_run);
    RUN_TEST(test_write_not_program);
    RUN_TEST(test_interrupt_start);
    RUN_TEST(test_page2_second_start);
    RUN_TEST(test_combined_code);
    return UNITY_END();
}
