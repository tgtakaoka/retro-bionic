// Host-side test of how the MC68000 family's backtrace sorts program
// reads: `pio test -e native`.

#include <unity.h>

#include "mc68000/fetch_mc68000.h"

using namespace debugger::mc68000;

namespace {

using Kind = FetchMc68000::Kind;
constexpr auto O = FetchMc68000::OTHER;
constexpr auto L = FetchMc68000::LOOSE;
constexpr auto S = FetchMc68000::FLUSH;
constexpr auto F = FetchMc68000::FETCH;
constexpr auto P = FetchMc68000::OPERAND;

// A run as the ring holds it.
struct Ring {
    FetchMc68000::Cycle c[64];
    uint8_t n = 0;

    // |count| program reads from |addr|, |bytes| each.
    Ring &prog(uint32_t addr, uint8_t count, uint8_t bytes) {
        for (uint8_t i = 0; i < count; ++i)
            c[n++] = {addr + i * bytes, bytes, true};
        return *this;
    }
    Ring &data(uint32_t addr, uint8_t count, uint8_t bytes) {
        for (uint8_t i = 0; i < count; ++i)
            c[n++] = {addr + i * bytes, bytes, false};
        return *this;
    }
    void check(bool anchored, const Kind *want, uint8_t len) const {
        TEST_ASSERT_EQUAL_UINT8(len, n);
        Kind kind[64];
        FetchMc68000::classify(c, n, anchored, kind);
        TEST_ASSERT_EQUAL_UINT8_ARRAY(want, kind, n);
    }
};

void test_straight() {
    Ring r;
    r.prog(0x1000, 3, 2);
    static const Kind ANCHORED[] = {S, F, F};
    r.check(true, ANCHORED, 3);
    static const Kind LOOSE[] = {L, L, L};
    r.check(false, LOOSE, 3);
}

void test_branch() {
    Ring r;
    r.prog(0x1000, 4, 2).prog(0x2000, 2, 2);
    static const Kind WANT[] = {S, F, F, F, S, F};
    r.check(true, WANT, 6);
}

// Until a flush, no fetch is known to start an instruction.
void test_loose_then_branch() {
    Ring r;
    r.prog(0x1000, 2, 2).prog(0x2000, 2, 2);
    static const Kind WANT[] = {L, L, S, F};
    r.check(false, WANT, 4);
}

// The vector is a data read; the handler's first fetch is a flush.
void test_exception() {
    Ring r;
    r.prog(0x1000, 4, 2).data(0x0FEFF8, 3, 2).data(0x00007C, 2, 2);
    r.prog(0x2000, 2, 2);
    static const Kind WANT[] = {S, F, F, F, O, O, O, O, O, S, F};
    r.check(true, WANT, 11);
}

void test_pcrel_word() {
    Ring r;
    r.prog(0x1000, 3, 2).prog(0x1040, 1, 2).prog(0x1006, 2, 2);
    static const Kind WANT[] = {S, F, F, P, F, F};
    r.check(true, WANT, 6);
}

void test_pcrel_long() {
    Ring r;
    r.prog(0x1000, 3, 2).prog(0x1040, 2, 2).prog(0x1006, 1, 2);
    static const Kind WANT[] = {S, F, F, P, P, F};
    r.check(true, WANT, 6);
}

// The MC68008 reads a long as four bytes.
void test_pcrel_long_8bit() {
    Ring r;
    r.prog(0x1000, 6, 1).prog(0x1040, 4, 1).prog(0x1006, 2, 1);
    static const Kind WANT[] = {S, F, F, F, F, F, P, P, P, P, F, F};
    r.check(true, WANT, 12);
}

// MOVEM from a PC-relative source: many reads, then the stream again.
void test_pcrel_movem() {
    Ring r;
    r.prog(0x1000, 4, 2).prog(0x1100, 12, 2).prog(0x1008, 1, 2);
    Kind want[17] = {S, F, F, F};
    for (uint8_t i = 4; i < 16; ++i)
        want[i] = P;
    want[16] = F;
    r.check(true, want, 17);
}

// An operand read stays loose around it.
void test_loose_operand() {
    Ring r;
    r.prog(0x1000, 2, 2).prog(0x1040, 1, 2).prog(0x1004, 1, 2);
    static const Kind WANT[] = {L, L, P, L};
    r.check(false, WANT, 4);
}

// A run that reaches the ring's end never came back: a flush.
void test_ring_end() {
    Ring r;
    r.prog(0x1000, 2, 2).prog(0x3000, 2, 2);
    static const Kind WANT[] = {S, F, S, F};
    r.check(true, WANT, 4);
}

}  // namespace

void setUp() {}
void tearDown() {}

int main() {
    UNITY_BEGIN();
    RUN_TEST(test_straight);
    RUN_TEST(test_branch);
    RUN_TEST(test_loose_then_branch);
    RUN_TEST(test_exception);
    RUN_TEST(test_pcrel_word);
    RUN_TEST(test_pcrel_long);
    RUN_TEST(test_pcrel_long_8bit);
    RUN_TEST(test_pcrel_movem);
    RUN_TEST(test_loose_operand);
    RUN_TEST(test_ring_end);
    return UNITY_END();
}

// Local Variables:
// mode: c++
// c-basic-offset: 4
// tab-width: 4
// End:
// vim: set ft=cpp et ts=4 sw=4:
