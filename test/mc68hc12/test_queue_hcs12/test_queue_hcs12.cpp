// Host-side test of the HCS12 queue replay: `pio test -e native`.
//
// The cycles follow S12CPUV2 8.4: the execution start, sampled before the
// E rise, begins this cycle with the opcode at the head of stage 3; an ALD,
// sampled before the E fall, takes this cycle's word into stage 1.

#include <unity.h>

#include "mc68hc12/queue_hcs12.cpp"
#include "mc68hc12/queue_mc68hc12.cpp"

using namespace debugger;
using namespace debugger::mc68hc12;

namespace {

Cycles *ring = nullptr;

constexpr uint8_t NONE = 0;
constexpr auto ALD = Signals::MOVE_ALD;
constexpr auto INT = Signals::START_INT;
constexpr auto SEV = Signals::START_EVEN;
constexpr auto SOD = Signals::START_ODD;

// A read; |start| is the status before the E rise, |move| before the fall.
void fetch(uint16_t addr, uint16_t data, uint8_t start, uint8_t move) {
    Signals::put()->set(addr, data, true, start, move);
    Cycles::next();
}

void read(uint16_t addr, uint8_t start = NONE, uint8_t move = NONE) {
    fetch(addr, 0, start, move);
}

void write(uint16_t addr, uint8_t start = NONE, uint8_t move = NONE) {
    Signals::put()->set(addr, 0, false, start, move, false);
    Cycles::next();
}

Signals *at(uint_fast8_t i) {
    return Signals::get()->next(i);
}

void replay() {
    const auto g = Signals::get();
    QueueHcs12().replay(g, g->diff(Signals::put()));
}

// The reset's VfPPP: the vector, a free cycle and three program words,
// then the first instruction. The status may be wrong in the first cycle
// or two after reset (S12CPUV2 8.4.2); here it is junk.
void resetSequence(uint8_t start, uint16_t org = 0x1000) {
    read(0xFFFE, SOD, ALD);             // 0 V, junk status
    read(0xFFFE, SEV);                  // 1 f, junk status
    fetch(org, 0xA7A7, NONE, ALD);      // 2 P
    fetch(org + 2, 0xA7A7, NONE, ALD);  // 3 P
    fetch(org + 4, 0xA7A7, NONE, ALD);  // 4 P
    read(0x2000, start);                // 5 the first instruction
}

}  // namespace

void setUp() {
    ring = new Cycles();
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

// The junk on the V cycle reaches no instruction: the queue is empty.
void test_junk_after_reset_ignored() {
    resetSequence(SEV);
    replay();
    TEST_ASSERT_FALSE(at(0)->fetch());
    TEST_ASSERT_FALSE(at(1)->fetch());
    TEST_ASSERT_TRUE(at(0)->program());  // its ALD took the vector
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

int main(int, char **) {
    UNITY_BEGIN();
    RUN_TEST(test_reset_even_start);
    RUN_TEST(test_reset_odd_start);
    RUN_TEST(test_junk_after_reset_ignored);
    RUN_TEST(test_nop_run);
    RUN_TEST(test_write_not_program);
    RUN_TEST(test_interrupt_start);
    RUN_TEST(test_page2_second_start);
    return UNITY_END();
}
