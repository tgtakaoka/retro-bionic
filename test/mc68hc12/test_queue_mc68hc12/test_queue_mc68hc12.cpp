// Host-side test of the M68HC12 queue replay: `pio test -e native`.
//
// The cycles follow CPU12RM 8.8: a movement at a cycle's E rise takes the
// previous cycle's word; a start at its E fall begins the next cycle with
// the opcode in queue stage 2.

#include <unity.h>

#include "mc68hc12/queue_mc68hc12.cpp"

using namespace debugger;
using namespace debugger::mc68hc12;

namespace {

Cycles *ring = nullptr;

constexpr uint8_t NONE = 0;
constexpr auto LAT = Signals::MOVE_LAT;
constexpr auto ALD = Signals::MOVE_ALD;
constexpr auto ALL = Signals::MOVE_ALL;
constexpr auto INT = Signals::START_INT;
constexpr auto SEV = Signals::START_EVEN;
constexpr auto SOD = Signals::START_ODD;

void read(uint16_t addr, uint8_t move, uint8_t start = NONE) {
    Signals::put()->set(addr, 0, true, move, start);
    Cycles::next();
}

void freeCycle(uint8_t move, uint8_t start = NONE) {
    Signals::put()->set(0xFFFF, 0, true, move, start, false);
    Cycles::next();
}

void write(uint16_t addr, uint8_t move, uint8_t start = NONE) {
    Signals::put()->set(addr, 0, false, move, start, false);
    Cycles::next();
}

Signals *at(uint_fast8_t i) {
    return Signals::get()->next(i);
}

void replay() {
    const auto g = Signals::get();
    Queue().replay(g, g->diff(Signals::put()));
}

// The reset's VfPPP: the vector, a free cycle and three program words;
// the first instruction starts at |start|.
void resetSequence(uint8_t start) {
    read(0xFFFE, NONE);        // 0 V
    freeCycle(NONE);           // 1 f
    read(0x1000, NONE);        // 2 P
    read(0x1002, ALD);         // 3 P, takes 1000
    read(0x1004, ALD, start);  // 4 P, takes 1002; starts next
    read(0x1006, ALD);         // 5 first instruction, takes 1004
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
    TEST_ASSERT_FALSE(at(1)->program());
}

void test_reset_odd_start() {
    resetSequence(SOD);
    replay();
    TEST_ASSERT_TRUE(at(5)->fetch());
    TEST_ASSERT_EQUAL_HEX16(0x1001, at(5)->inst());
}

// A start before two words reached the queue can't be placed.
void test_start_before_queue_filled() {
    read(0x1000, NONE);
    read(0x1002, ALD, SEV);  // stage 2 still empty
    read(0x1004, ALD);
    replay();
    TEST_ASSERT_FALSE(at(2)->fetch());
}

// A word that arrives before the queue can advance is latched, and moves
// in with ALL.
void test_latch_then_advance_from_latch() {
    resetSequence(SEV);      // 0-5: st2=1002, st1=1004 after cycle 5
    read(0x1008, LAT, SOD);  // 6: latch 1006; an opcode at 1003 next
    read(0x100A, ALL, SEV);  // 7: st2=1004, st1=1006 from the latch
    read(0x100C, ALD, SEV);  // 8: st2=1006, st1=100A
    read(0x100E, NONE);      // 9
    replay();
    TEST_ASSERT_TRUE(at(7)->fetch());
    TEST_ASSERT_EQUAL_HEX16(0x1003, at(7)->inst());
    TEST_ASSERT_TRUE(at(8)->fetch());
    TEST_ASSERT_EQUAL_HEX16(0x1004, at(8)->inst());
    TEST_ASSERT_TRUE(at(9)->fetch());
    TEST_ASSERT_EQUAL_HEX16(0x1006, at(9)->inst());
}

// A second LAT before the queue advances is ignored: the latch keeps the
// first word.
void test_second_latch_ignored() {
    resetSequence(SEV);      // 0-5: st2=1002, st1=1004
    read(0x2000, LAT);       // 6: latch 1006 (cycle 5)
    read(0x2002, LAT);       // 7: ignored (it would be 2000)
    read(0x2004, ALL);       // 8: st2=1004, st1=1006
    read(0x2006, ALD, SEV);  // 9: st2=1006, st1=2004
    read(0x2008, NONE);      // 10
    replay();
    TEST_ASSERT_TRUE(at(10)->fetch());
    TEST_ASSERT_EQUAL_HEX16(0x1006, at(10)->inst());
    TEST_ASSERT_FALSE(at(6)->program());
}

// A write is never a program word, even where a movement follows it.
void test_write_not_program() {
    resetSequence(SEV);
    write(0x0800, NONE);  // 6
    read(0x1008, ALD);    // 7: takes the write: no word
    replay();
    TEST_ASSERT_FALSE(at(6)->program());
}

void test_interrupt_start() {
    resetSequence(SEV);
    read(0x1008, ALD, INT);  // 6: an interrupt sequence begins at 7
    read(0xFFF4, NONE);      // 7: the vector
    replay();
    TEST_ASSERT_TRUE(at(7)->interrupt());
    TEST_ASSERT_FALSE(at(7)->fetch());
}

int main(int, char **) {
    UNITY_BEGIN();
    RUN_TEST(test_reset_even_start);
    RUN_TEST(test_reset_odd_start);
    RUN_TEST(test_start_before_queue_filled);
    RUN_TEST(test_latch_then_advance_from_latch);
    RUN_TEST(test_second_latch_ignored);
    RUN_TEST(test_write_not_program);
    RUN_TEST(test_interrupt_start);
    return UNITY_END();
}
