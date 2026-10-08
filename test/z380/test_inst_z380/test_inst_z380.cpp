// Host-side test of the Z380 bus walker: `pio test -e native`.
//
// z380.cycles.zst holds bench rings and recorded runs (see
// debugger/z380/tools/rings_z380.py); z380.walks.zst what the walker made
// of each, written by MATCH_GOLDEN=<dir> pio test -e native, a record per
// ring: its name and walk. Every instruction walked in a bench ring starts
// a line of its listing.

#include "../../match_harness.h"

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

struct Ring {
    std::string name;
    bool xm;
    bool lw;
    uint32_t stop;
    std::string cycles;
    std::string memory;
    std::string starts;  // the listing's instruction starts, if a bench's
};

// The fixtures: next to this file.
const auto DIR = match_harness::dirOf(__FILE__);

// z380.cycles.zst's: name, xm, lw, stop, cycles, memory, starts.
std::vector<Ring> loadRings() {
    std::vector<Ring> rings;
    for (const auto &f : match_harness::records(DIR + "z380.cycles.zst")) {
        if (f.size() < 7)
            match_harness::broken(DIR + "z380.cycles.zst", "a short ring");
        rings.push_back(Ring{f[0], f[1] == "1", f[2] == "1",
                uint32_t(strtoul(f[3].c_str(), nullptr, 16)), f[4], f[5],
                f[6]});
    }
    return rings;
}

const auto RINGS = loadRings();

// Each ring's walk: "@start addr !vector ...", or "no walk".
std::map<std::string, std::string> loadWalks() {
    std::map<std::string, std::string> walks;
    for (const auto &f : match_harness::records(DIR + "z380.walks.zst")) {
        if (f.size() == 2)
            walks[f[0]] = f[1];
    }
    return walks;
}

const auto WALKS = loadWalks();

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

const MatchWalker &walker = MatchWalker::shared();

// "@start addr addr": an interrupt as ! and where it goes.
std::string walked(bool ok) {
    if (!ok)
        return "no walk";
    std::string got;
    char buf[16];
    snprintf(buf, sizeof(buf), "@%u", walker.start());
    got = buf;
    for (auto n = 0u; n < walker.steps(); ++n) {
        if (walker.interrupt(n)) {
            if (n + 1 < walker.steps())
                snprintf(buf, sizeof(buf), " !%X", walker.addr(n + 1));
            else
                snprintf(buf, sizeof(buf), " !");
        } else {
            snprintf(buf, sizeof(buf), " %X", walker.addr(n));
        }
        got += buf;
    }
    return got;
}

std::string *out;   // MATCH_GOLDEN=<dir>: the walks made, to z380.walks.zst
unsigned unlisted;  // bench rings walked off the listing

void check(const Ring &w) {
    Cycles::reset();
    Memory memory;
    load(memory, w.memory.c_str());
    const auto begin = feed(w.cycles.c_str());
    const auto end = Signals::put();
    // WALK_TRACE=<name>: print that walk's failures
    const auto only = getenv("WALK_TRACE");
    MatchWalker::trace = only && w.name == only;
    const auto got =
            walked(InstZ380::walk(begin, end, memory, w.stop, w.xm, w.lw));
    MatchWalker::trace = false;
    if (out) {
        *out += w.name + "\t" + got + "\n";
        return;
    }
    const auto golden = WALKS.find(w.name);
    TEST_ASSERT_TRUE_MESSAGE(golden != WALKS.end(), w.name.c_str());
    TEST_ASSERT_EQUAL_STRING_MESSAGE(
            golden->second.c_str(), got.c_str(), w.name.c_str());
    // a bench ring's instructions start the listing's lines
    if (w.starts.empty())
        return;
    std::string starts = std::string(" ") + w.starts + " ";
    std::string off;
    for (auto n = 0u; n < walker.steps(); ++n) {
        char addr[16];
        snprintf(addr, sizeof(addr), " %X ", walker.addr(n));
        if (!walker.interrupt(n) && starts.find(addr) == std::string::npos)
            off += addr + 1;
    }
    if (!off.empty()) {
        printf("%s: not in the listing: %s\n", w.name.c_str(), off.c_str());
        ++unlisted;
    }
}

}  // namespace

void setUp() {
    Cycles::reset();
}

void tearDown() {}

void test_walks() {
    TEST_ASSERT_FALSE_MESSAGE(RINGS.empty(), "no z380.cycles.zst");
    const auto dir = getenv("MATCH_GOLDEN");
    std::string walks =
            "# Generated by: MATCH_GOLDEN=<dir> pio test -e native\n";
    if (dir)
        out = &walks;
    for (const auto &w : RINGS)
        check(w);
    if (out)
        match_harness::writeZst(std::string(dir) + "/z380.walks.zst", walks);
    out = nullptr;
    TEST_ASSERT_EQUAL_UINT_MESSAGE(0, unlisted, "bench rings off the listing");
}

// Every cycle of a walk belongs to an instruction, and each one's first
// fetch is marked.
void test_marks() {
    const auto &w = RINGS[0];
    Cycles::reset();
    Memory memory;
    load(memory, w.memory.c_str());
    const auto begin = feed(w.cycles.c_str());
    const auto end = Signals::put();
    TEST_ASSERT_TRUE(InstZ380::walk(begin, end, memory, w.stop, w.xm, w.lw));
    auto fetches = 0u;
    for (auto i = walker.start(); i < begin->diff(end); ++i) {
        TEST_ASSERT_NOT_EQUAL(MatchWalker::NOBODY, walker.owner(i));
        fetches += begin->next(i)->fetch();
    }
    TEST_ASSERT_GREATER_THAN(walker.steps() / 2, fetches);
}

// A ring with no walk to the stop: nothing walked, nothing marked.
void test_no_walk() {
    Memory memory;
    const auto begin = cycle(MRD, 0x0100, 0x0000, true);
    cycle(MWR, 0x2000, 0x1234, true);
    cycle(MWR, 0x2002, 0x1234, true);
    const auto end = Signals::put();
    TEST_ASSERT_FALSE(InstZ380::walk(begin, end, memory, 0x0100, false, false));
    TEST_ASSERT_EQUAL_UINT(0, walker.steps());
    for (auto i = 0u; i < begin->diff(end); ++i)
        TEST_ASSERT_FALSE(begin->next(i)->fetch());
}

// Every sequence it walks with is the legend's, but the address bytes a
// DDIR's immediate widens.
void test_sequences() {
    match_harness::checkSequences(InstZ380::sequences(), "a");
    for (const auto seq : InstZ380::interrupts())
        match_harness::checkInterrupt(seq);
}

int main() {
    UNITY_BEGIN();
    RUN_TEST(test_sequences);
    RUN_TEST(test_walks);
    RUN_TEST(test_marks);
    RUN_TEST(test_no_walk);
    return UNITY_END();
}
