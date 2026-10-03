// Host-side test of the Z380 bus walker: `pio test -e native`.
//
// walks.inc holds bench rings and recorded runs, each with what
// debugger/z380/tools/walk_z380.py made of it; the firmware's walker must
// make the same.

#include <unity.h>

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <map>
#include <string>

#define protected public  // the ring is fed straight into _signals
#include "z380/signals_z380.h"
#undef protected
#include "z380/inst_z380.h"

using namespace debugger;
using namespace debugger::z380;

namespace debugger {
namespace z380 {
void Signals::print() const {}  // the real one needs the console
}  // namespace z380
}  // namespace debugger

namespace {

// Strobes, set when asserted; cycle() stores them as the pins read.
constexpr uint8_t M1 = 0x01, MRD = 0x02, MWR = 0x04, IORD = 0x08, IOWR = 0x10;
// #BLEN and #BHEN as the pins read: set when negated.
constexpr uint8_t BLEN = 0x01, BHEN = 0x02;

struct Walk {
    const char *name;
    bool xm;
    bool lw;
    uint32_t stop;
    unsigned start;  // the cycle the walk starts from
    const char *cycles;
    const char *memory;
    const char *expected;
};

const Walk WALKS[] = {
#include "walks.inc"
};

struct Memory final : InstZ380::Memory {
    std::map<uint32_t, uint8_t> mem;
    uint16_t read_byte(uint32_t a) const override {
        const auto it = mem.find(a & 0xFFFFFF);
        return it == mem.end() ? 0xFF : it->second;
    }
};

Signals *cycle(uint8_t cntl, uint32_t addr, uint16_t data, bool word) {
    auto s = Signals::put();
    s->clear();
    s->clearMark();
    s->addr = addr;
    s->data = data;
    s->_signals[0] = ~cntl & 0x1F;  // the pins: low when asserted
    s->_signals[1] = (word || (addr & 1) == 0 ? 0 : BHEN) |
                     (word || (addr & 1) != 0 ? 0 : BLEN);
    Cycles::next();
    return s;
}

// "R12C=40C9 r41=0000.": kind, address, data, . for a byte.
Signals *feed(const char *text) {
    Signals *begin = nullptr;
    const char *p = text;
    while (*p) {
        const char kind = *p++;
        char *q;
        const uint32_t addr = strtoul(p, &q, 16);
        const uint16_t data = strtoul(q + 1, &q, 16);
        bool word = true;
        if (*q == '.') {
            word = false;
            ++q;
        }
        const uint8_t cntl = kind == 'R'   ? MRD
                             : kind == 'W' ? MWR
                             : kind == 'r' ? IORD
                             : kind == 'w' ? IOWR
                                           : M1;
        const auto s = cycle(cntl, addr, data, word);
        if (begin == nullptr)
            begin = s;
        p = q;
        while (*p == ' ')
            ++p;
    }
    return begin;
}

// "100:FD18191A": byte runs.
void load(Memory &m, const char *text) {
    const char *p = text;
    while (*p) {
        char *q;
        auto addr = strtoul(p, &q, 16);
        for (p = q + 1; isxdigit(p[0]) && isxdigit(p[1]); p += 2) {
            const char hex[3] = {p[0], p[1], 0};
            m.mem[addr++] = strtoul(hex, nullptr, 16);
        }
        while (*p == ' ')
            ++p;
    }
}

InstZ380 inst;  // big: off the stack

void check(const Walk &w) {
    Cycles::reset();
    Memory memory;
    load(memory, w.memory);
    const auto begin = feed(w.cycles);
    const auto end = Signals::put();
    std::string expected = w.expected;
    std::string got;
    char buf[16];
    // Z380_TRACE=<name>: print that walk's failures
    const auto only = getenv("Z380_TRACE");
    InstZ380::trace = only && strcmp(only, w.name) == 0;
    if (inst.walk(begin, end, memory, w.stop, w.xm, w.lw)) {
        snprintf(buf, sizeof(buf), "@%u ", begin->diff(inst.start()));
        got = buf;
        for (auto n = 0u; n < inst.steps(); ++n) {
            snprintf(buf, sizeof(buf), "%s%s%X", n ? " " : "",
                    inst.interrupt(n) ? "!" : "", inst.addr(n));
            got += buf;
        }
    } else {
        got = "no walk";
    }
    snprintf(buf, sizeof(buf), "@%u ", w.start);
    expected = buf + expected;
    TEST_ASSERT_EQUAL_STRING_MESSAGE(expected.c_str(), got.c_str(), w.name);
}

}  // namespace

void setUp() {
    Cycles::reset();
}

void tearDown() {}

void test_walks() {
    for (const auto &w : WALKS)
        check(w);
}

// Every cycle of a walk belongs to an instruction, and each one's first
// fetch is marked.
void test_marks() {
    const auto &w = WALKS[0];
    Cycles::reset();
    Memory memory;
    load(memory, w.memory);
    const auto begin = feed(w.cycles);
    const auto end = Signals::put();
    TEST_ASSERT_TRUE(inst.walk(begin, end, memory, w.stop, w.xm, w.lw));
    const auto from = begin->diff(inst.start());
    auto fetches = 0u;
    for (auto i = from; i < begin->diff(end); ++i) {
        TEST_ASSERT_NOT_EQUAL(InstZ380::NOBODY, inst.owner(i));
        fetches += begin->next(i)->fetch();
    }
    TEST_ASSERT_GREATER_THAN(inst.steps() / 2, fetches);
}

// A ring with no walk to the stop: nothing walked, nothing marked.
void test_no_walk() {
    Memory memory;
    const auto begin = cycle(MRD, 0x0100, 0x0000, true);
    cycle(MWR, 0x2000, 0x1234, true);
    cycle(MWR, 0x2002, 0x1234, true);
    const auto end = Signals::put();
    TEST_ASSERT_FALSE(inst.walk(begin, end, memory, 0x0100, false, false));
    TEST_ASSERT_EQUAL_UINT(0, inst.steps());
    for (auto i = 0u; i < begin->diff(end); ++i)
        TEST_ASSERT_FALSE(begin->next(i)->fetch());
}

int main() {
    UNITY_BEGIN();
    RUN_TEST(test_walks);
    RUN_TEST(test_marks);
    RUN_TEST(test_no_walk);
    return UNITY_END();
}
