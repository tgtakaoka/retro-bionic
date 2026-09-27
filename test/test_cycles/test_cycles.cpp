// Host-side tests for the bus cycle ring: `pio test -e native`.

#include <unity.h>

#include "signals.h"

using namespace debugger;

namespace {

// The minimal concrete Signals type: what every target derives.
struct Sig : SignalsBase<Sig> {};

constexpr auto MAX = Cycles::MAX_CYCLES;

Cycles *ring = nullptr;

uint_fast8_t recorded() {
    return Sig::get()->diff(Sig::put());
}

void advance(unsigned n) {
    while (n--)
        Cycles::next();
}

}  // namespace

void setUp() {
    ring = new Cycles();
}

void tearDown() {
    delete ring;
    ring = nullptr;
}

void test_reset_is_empty() {
    TEST_ASSERT_EQUAL(0, Cycles::cycles());
    TEST_ASSERT_EQUAL_PTR(Cycles::at(0), Cycles::head());
    TEST_ASSERT_EQUAL_PTR(Cycles::at(0), Cycles::tail());
    TEST_ASSERT_EQUAL(0, recorded());
}

void test_next_counts_completed_cycles() {
    advance(10);
    TEST_ASSERT_EQUAL(10, Cycles::cycles());
    TEST_ASSERT_EQUAL(10, recorded());
    TEST_ASSERT_EQUAL_PTR(Cycles::at(0), Cycles::tail());
    TEST_ASSERT_EQUAL_PTR(Cycles::at(10), Cycles::head());
}

void test_next_clears_the_new_head() {
    Sig::put()->inject(0x1234)->capture();
    TEST_ASSERT_FALSE(Sig::put()->readMemory());
    TEST_ASSERT_FALSE(Sig::put()->writeMemory());
    Cycles::next();
    TEST_ASSERT_TRUE(Sig::put()->readMemory());
    TEST_ASSERT_TRUE(Sig::put()->writeMemory());
    // The completed one keeps its marks and its data.
    TEST_ASSERT_FALSE(Sig::get()->readMemory());
    TEST_ASSERT_EQUAL_HEX16(0x1234, Sig::get()->data);
}

// The ring holds one fewer completed cycle than it has slots: the head is
// the transaction in progress.
void test_ring_holds_max_minus_one() {
    advance(MAX - 1);
    TEST_ASSERT_EQUAL(MAX - 1, Cycles::cycles());
    TEST_ASSERT_EQUAL(MAX - 1, recorded());
    TEST_ASSERT_EQUAL_PTR(Cycles::at(0), Cycles::tail());
    TEST_ASSERT_EQUAL_PTR(Cycles::at(MAX - 1), Cycles::head());
}

// Exactly MAX_CYCLES completed once left tail and head on the same slot,
// so a dump of that many cycles printed nothing.
void test_exactly_max_cycles_still_dumps() {
    advance(MAX);
    TEST_ASSERT_EQUAL(MAX - 1, recorded());
    TEST_ASSERT_EQUAL_PTR(Cycles::at(0), Cycles::head());
    TEST_ASSERT_EQUAL_PTR(Cycles::at(1), Cycles::tail());
}

void test_wrapped_ring_keeps_the_newest() {
    advance(3 * MAX + 5);
    TEST_ASSERT_EQUAL(MAX - 1, recorded());
    TEST_ASSERT_EQUAL_PTR(Cycles::at(5), Cycles::head());
    TEST_ASSERT_EQUAL_PTR(Cycles::at(6), Cycles::tail());
    TEST_ASSERT_EQUAL_PTR(Cycles::head(), Sig::get()->next(MAX - 1));
}

void test_discard_rewinds_the_head() {
    advance(10);
    Cycles::discard(Cycles::at(6));
    TEST_ASSERT_EQUAL(6, Cycles::cycles());
    TEST_ASSERT_EQUAL(6, recorded());
    TEST_ASSERT_EQUAL_PTR(Cycles::at(6), Cycles::head());
    TEST_ASSERT_EQUAL_PTR(Cycles::at(0), Cycles::tail());
}

void test_discard_head_is_a_no_op() {
    advance(10);
    Cycles::discard(Cycles::head());
    TEST_ASSERT_EQUAL(10, Cycles::cycles());
    TEST_ASSERT_EQUAL_PTR(Cycles::at(10), Cycles::head());
}

void test_discard_clears_the_slot_it_lands_on() {
    advance(3);
    Cycles::at(1)->clear();
    static_cast<Sig *>(Cycles::at(1))->inject(0);
    Cycles::discard(Cycles::at(1));
    TEST_ASSERT_TRUE(Sig::put()->readMemory());
}

// Further back than the ring holds: everything goes.
void test_discard_past_the_tail_empties() {
    advance(5);
    Cycles::discard(Cycles::at(120));
    TEST_ASSERT_EQUAL(0, Cycles::cycles());
    TEST_ASSERT_EQUAL(0, recorded());
    TEST_ASSERT_EQUAL_PTR(Cycles::tail(), Cycles::head());
}

void test_discard_across_the_wrap() {
    advance(MAX + 4);  // head at 4, tail at 5
    Cycles::discard(Cycles::at(MAX - 2));
    TEST_ASSERT_EQUAL_PTR(Cycles::at(MAX - 2), Cycles::head());
    TEST_ASSERT_EQUAL(MAX - 1 - 6, recorded());
    TEST_ASSERT_EQUAL_PTR(Cycles::at(5), Cycles::tail());
    // And recording continues from there.
    advance(2);
    TEST_ASSERT_EQUAL_PTR(Cycles::at(0), Cycles::head());
    TEST_ASSERT_EQUAL(MAX - 1 - 4, recorded());
}

// prev()/next() walk the ring by slot, whatever is recorded: they are how
// the exit paths look back from a fetch to the write before it.
void test_next_and_prev_step_by_slot() {
    const auto s = static_cast<Sig *>(Cycles::at(10));
    TEST_ASSERT_EQUAL_PTR(Cycles::at(11), s->next());
    TEST_ASSERT_EQUAL_PTR(Cycles::at(13), s->next(3));
    TEST_ASSERT_EQUAL_PTR(Cycles::at(9), s->prev());
    TEST_ASSERT_EQUAL_PTR(Cycles::at(7), s->prev(3));
    TEST_ASSERT_EQUAL_PTR(s, s->next(0));
    TEST_ASSERT_EQUAL_PTR(s, s->prev(0));
    TEST_ASSERT_EQUAL_PTR(s, s->next(4)->prev(4));
}

void test_next_and_prev_wrap() {
    const auto first = static_cast<Sig *>(Cycles::at(0));
    const auto last = static_cast<Sig *>(Cycles::at(MAX - 1));
    TEST_ASSERT_EQUAL_PTR(last, first->prev());
    TEST_ASSERT_EQUAL_PTR(Cycles::at(MAX - 2), first->prev(2));
    TEST_ASSERT_EQUAL_PTR(first, last->next());
    TEST_ASSERT_EQUAL_PTR(Cycles::at(1), last->next(2));
    TEST_ASSERT_EQUAL_PTR(first, first->next(MAX));
    TEST_ASSERT_EQUAL_PTR(first, first->prev(MAX));
}

void test_prev_from_the_head_is_the_last_completed() {
    advance(3);
    Sig::put()->inject(0xABCD);
    Cycles::next();
    TEST_ASSERT_EQUAL_HEX16(0xABCD, Sig::put()->prev()->data);
    TEST_ASSERT_EQUAL_PTR(Cycles::at(3), Sig::put()->prev());
    TEST_ASSERT_EQUAL_PTR(Sig::get(), Sig::put()->prev(4));
}

// diff() is the distance forward from this slot to another, modulo the
// ring: tail->diff(head) is how many cycles a dump prints.
void test_diff_forward() {
    TEST_ASSERT_EQUAL(0, Cycles::at(7)->diff(Cycles::at(7)));
    TEST_ASSERT_EQUAL(1, Cycles::at(7)->diff(Cycles::at(8)));
    TEST_ASSERT_EQUAL(5, Cycles::at(3)->diff(Cycles::at(8)));
    TEST_ASSERT_EQUAL(MAX - 1, Cycles::at(0)->diff(Cycles::at(MAX - 1)));
}

void test_diff_wraps() {
    TEST_ASSERT_EQUAL(1, Cycles::at(MAX - 1)->diff(Cycles::at(0)));
    TEST_ASSERT_EQUAL(MAX - 5, Cycles::at(8)->diff(Cycles::at(3)));
    TEST_ASSERT_EQUAL(MAX - 1, Cycles::at(1)->diff(Cycles::at(0)));
    TEST_ASSERT_EQUAL(MAX - 1, Cycles::at(MAX - 1)->diff(Cycles::at(MAX - 2)));
}

void test_pos_and_at_agree() {
    for (unsigned i = 0; i < MAX; i++)
        TEST_ASSERT_EQUAL(i, Cycles::at(i)->pos());
    TEST_ASSERT_EQUAL_PTR(Cycles::at(3), Cycles::at(MAX + 3));
    TEST_ASSERT_EQUAL(3, Cycles::indexOf(Cycles::at(3)));
}

// cycles() is the completed count, saturating at MAX_CYCLES - 1; what a
// dump prints, tail->diff(head), tracks it exactly.
void test_count_tracks_next_and_discard() {
    TEST_ASSERT_EQUAL(0, Cycles::cycles());
    advance(1);
    TEST_ASSERT_EQUAL(1, Cycles::cycles());
    TEST_ASSERT_EQUAL(1, recorded());
    advance(MAX - 3);
    TEST_ASSERT_EQUAL(MAX - 2, Cycles::cycles());
    TEST_ASSERT_EQUAL(MAX - 2, recorded());
    advance(1);
    TEST_ASSERT_EQUAL(MAX - 1, Cycles::cycles());
    TEST_ASSERT_EQUAL(MAX - 1, recorded());
    advance(1);  // full: the oldest goes, the count stays
    TEST_ASSERT_EQUAL(MAX - 1, Cycles::cycles());
    TEST_ASSERT_EQUAL(MAX - 1, recorded());
    Cycles::discard(Sig::put()->prev(10));
    TEST_ASSERT_EQUAL(MAX - 11, Cycles::cycles());
    TEST_ASSERT_EQUAL(MAX - 11, recorded());
    Cycles::reset();
    TEST_ASSERT_EQUAL(0, Cycles::cycles());
    TEST_ASSERT_EQUAL(0, recorded());
}

void test_count_matches_dump_at_every_length() {
    for (unsigned n = 0; n < 3 * MAX; n++) {
        const auto expect = n < MAX - 1 ? n : MAX - 1;
        TEST_ASSERT_EQUAL(expect, Cycles::cycles());
        TEST_ASSERT_EQUAL(expect, recorded());
        Cycles::next();
    }
}

int main() {
    UNITY_BEGIN();
    RUN_TEST(test_reset_is_empty);
    RUN_TEST(test_next_counts_completed_cycles);
    RUN_TEST(test_next_clears_the_new_head);
    RUN_TEST(test_ring_holds_max_minus_one);
    RUN_TEST(test_exactly_max_cycles_still_dumps);
    RUN_TEST(test_wrapped_ring_keeps_the_newest);
    RUN_TEST(test_discard_rewinds_the_head);
    RUN_TEST(test_discard_head_is_a_no_op);
    RUN_TEST(test_discard_clears_the_slot_it_lands_on);
    RUN_TEST(test_discard_past_the_tail_empties);
    RUN_TEST(test_discard_across_the_wrap);
    RUN_TEST(test_next_and_prev_step_by_slot);
    RUN_TEST(test_next_and_prev_wrap);
    RUN_TEST(test_prev_from_the_head_is_the_last_completed);
    RUN_TEST(test_diff_forward);
    RUN_TEST(test_diff_wraps);
    RUN_TEST(test_pos_and_at_agree);
    RUN_TEST(test_count_tracks_next_and_discard);
    RUN_TEST(test_count_matches_dump_at_every_length);
    return UNITY_END();
}
