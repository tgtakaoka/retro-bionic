// Host-side test of the MC68HC16's IPIPE pipeline tracker and address mux
// composition: `pio test -e native`.

#include <unity.h>

#include "mc68hc16/pipe_mc68hc16.cpp"
#include "mc68hc16/signals_mc68hc16.h"

using namespace debugger::mc68hc16;

namespace {

// IPIPE1:IPIPE0, active low (CPU16RM Table 10-1).
constexpr uint8_t START_FETCH = 0, FETCH = 1, START = 2, NUL = 3;
constexpr uint8_t INVALID = 0, ADVANCE = 1, EXCEPTION = 2;

PipeMc68hc16::Cycle cycle(uint8_t p1, uint8_t p2, bool program = true) {
    return PipeMc68hc16::Cycle{p1, p2, program, false};
}

constexpr int16_t NO = PipeMc68hc16::NOT_START;

// Words fetched into stage A move to stage B, and START begins the one
// there: each start points back at its opcode's fetch.
void test_straight_line() {
    const PipeMc68hc16::Cycle c[] = {
            cycle(FETCH, NUL),            // 0: word 0 into A
            cycle(FETCH, ADVANCE),        // 1: word 0 to B, word 1 into A
            cycle(START_FETCH, ADVANCE),  // 2: word 0 starts
            cycle(START, NUL),            // 3: word 1 starts
            cycle(NUL, NUL, false),       // 4: data
    };
    int16_t starts[5];
    PipeMc68hc16::track(c, 5, starts);
    TEST_ASSERT_EQUAL(NO, starts[0]);
    TEST_ASSERT_EQUAL(NO, starts[1]);
    TEST_ASSERT_EQUAL(2, starts[2]);
    TEST_ASSERT_EQUAL(2, starts[3]);
    TEST_ASSERT_EQUAL(NO, starts[4]);
}

// An exception flushes the pipeline: a start right after knows no fetch.
void test_exception() {
    const PipeMc68hc16::Cycle c[] = {
            cycle(FETCH, NUL),
            cycle(FETCH, ADVANCE),
            cycle(NUL, EXCEPTION, false),
            cycle(START, NUL),
    };
    int16_t starts[4];
    PipeMc68hc16::track(c, 4, starts);
    TEST_ASSERT_EQUAL(0, starts[3]);
}

// A start before the ring's first fetch has no fetch to point at.
void test_unknown_fetch() {
    const PipeMc68hc16::Cycle c[] = {cycle(START, NUL)};
    int16_t starts[1];
    PipeMc68hc16::track(c, 1, starts);
    TEST_ASSERT_EQUAL(0, starts[0]);
}

// Acknowledges and cycles flagged INVALID carry no pipeline state.
void test_ignored() {
    PipeMc68hc16::Cycle ack = cycle(START_FETCH, ADVANCE, false);
    ack.iack = true;
    const PipeMc68hc16::Cycle c[] = {
            cycle(FETCH, NUL),
            ack,
            cycle(START_FETCH, INVALID),
            cycle(NUL, ADVANCE),
            cycle(START, NUL),
    };
    int16_t starts[5];
    PipeMc68hc16::track(c, 5, starts);
    TEST_ASSERT_EQUAL(NO, starts[1]);
    TEST_ASSERT_EQUAL(NO, starts[2]);
    TEST_ASSERT_EQUAL(4, starts[4]);
}

// The four mux reads compose A0-A19, FC2:FC0 and SIZ1:SIZ0.
void test_compose() {
    const auto a = Signals::compose(0x00030005,  // 00: A0-A3, A16-A19
            0x0003000A,                          // 01: A4-A7, A20-A23
            0x0006000C,                          // 11: A8-A11, FC
            0x00020001);                         // 10: A12-A15, SIZ
    TEST_ASSERT_EQUAL_HEX32(0x31CA5, a.addr);
    TEST_ASSERT_EQUAL(6, a.fc);
    TEST_ASSERT_EQUAL(2, a.siz);
}

}  // namespace

void setUp() {}
void tearDown() {}

int main() {
    UNITY_BEGIN();
    RUN_TEST(test_straight_line);
    RUN_TEST(test_exception);
    RUN_TEST(test_unknown_fetch);
    RUN_TEST(test_ignored);
    RUN_TEST(test_compose);
    return UNITY_END();
}
