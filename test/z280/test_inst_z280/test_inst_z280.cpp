// Host-side test of the Z280 cycle matcher: `pio test -e native`.
//
// <set>.cycles.zst are the recorded rings (see test/match_rings.py),
// <set>.marks.zst what findFetch() made of them (see
// test/match_harness.h).
//
// dumps/ holds verbose console dumps of sample runs on the board, with
// the HEX and listing of each sample as it was when recorded. Each dump
// is replayed through InstZ280::findFetch() and every mark checked
// against that listing, except the first lines, where a ring cut inside
// an instruction may still decode as a chain.
//
// Z280_DUMP=<file> [Z280_TRACE=1] replays that dump and prints every
// cycle's mark (I fetch, b byte or stale prefetch, o data). Sanitizers:
//   PLATFORMIO_BUILD_FLAGS='-fsanitize=address,undefined' pio test -e native

#include "../../match_harness.h"

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <map>
#include <string>
#include <vector>

#define protected public  // the dump is fed straight into _signals
#include "z280/signals_z280.h"
#undef protected
#include "z280/inst_z280.h"

using namespace debugger;
using namespace debugger::z280;

namespace debugger {
namespace z280 {
void Signals::print() const {}  // the real one needs the console
}  // namespace z280
}  // namespace debugger

namespace {

// The fixtures: next to this file.
const auto DIR = match_harness::dirOf(__FILE__);

std::string here() {
    std::string f = __FILE__;
    return f.substr(0, f.rfind('/') + 1);
}

// A flat memory loaded from a sample's Intel HEX.
struct Memory final : InstZ280::Memory {
    std::vector<uint8_t> mem = std::vector<uint8_t>(1 << 16, 0xFF);
    uint16_t read_byte(uint32_t a) const override { return mem[a & 0xFFFF]; }
    bool load(const std::string &path) {
        FILE *f = fopen(path.c_str(), "r");
        if (!f)
            return false;
        char line[600];
        while (fgets(line, sizeof line, f)) {
            if (line[0] != ':')
                continue;
            auto hex = [&](int i) {
                char b[3] = {line[i], line[i + 1], 0};
                return (uint8_t)strtoul(b, nullptr, 16);
            };
            const int n = hex(1);
            const uint32_t a = hex(3) << 8 | hex(5);
            if (hex(7) != 0)
                continue;
            for (int i = 0; i < n; ++i)
                mem[(a + i) & 0xFFFF] = hex(9 + 2 * i);
        }
        fclose(f);
        return true;
    }
};

// Every instruction start in a sample's listing, with its bytes.
std::map<uint32_t, std::string> listing(const std::string &path) {
    std::map<uint32_t, std::string> out;
    FILE *f = fopen(path.c_str(), "r");
    if (!f)
        return out;
    char line[400];
    while (fgets(line, sizeof line, f)) {
        const char *p = line;
        if (*p == '(' && isdigit(p[1]) && p[2] == ')')
            p += 3;
        while (*p == ' ')
            ++p;
        if (!isxdigit(*p))
            continue;
        char *end;
        const uint32_t addr = strtoul(p, &end, 16);
        if (strncmp(end, " : ", 3))
            continue;
        p = end + 3;
        std::string bytes;
        while (isxdigit(p[0]) && isxdigit(p[1]) &&
                (p[2] == ' ' || p[2] == '\n')) {
            bytes += p[0];
            bytes += p[1];
            p += 3;
        }
        while (*p == ' ')
            ++p;
        if (bytes.empty() || !isalpha(*p))
            continue;
        std::string mnemo;
        while (isalnum(*p))
            mnemo += tolower(*p++);
        if (mnemo == "org" || mnemo == "db" || mnemo == "dw" || mnemo == "ds" ||
                mnemo == "equ" || mnemo == "include" || mnemo == "end")
            continue;
        out[addr] = bytes;
    }
    fclose(f);
    return out;
}

// The dump into the ring; returns the PC it ended with.
uint32_t load_dump(const std::string &path) {
    FILE *f = fopen(path.c_str(), "r");
    TEST_ASSERT_NOT_NULL_MESSAGE(f, path.c_str());
    Cycles::reset();
    uint32_t pc = 0;
    char line[256];
    while (fgets(line, sizeof line, f)) {
        if (!strncmp(line, "PC=", 3)) {
            pc = strtoul(line + 3, nullptr, 16);
            continue;
        }
        // "  12  R A=..." from a profiling build, "R A=..." from a plain one.
        const char *p = line;
        while (*p == ' ' || *p == 'i' || *p == 'c')
            ++p;
        while (isdigit(*p))
            ++p;
        while (*p == ' ')
            ++p;
        char rw = *p++;
        if (rw == 'I')
            rw = 'R';
        if (rw != 'R' && rw != 'W')
            continue;
        while (*p == ' ')
            ++p;
        const bool io = *p == 'I';
        if (p[1] != '=')
            continue;
        p += 2;
        const uint32_t addr = strtoul(p, nullptr, 16);
        p += 6;
        while (*p == ' ')
            ++p;
        if (strncmp(p, "D=", 2))
            continue;
        p += 2;
        // A byte rides one lane, printed on its side; the other is blank.
        const bool byte = p[0] == ' ' || p[2] == ' ';
        uint16_t data;
        if (!byte) {
            data = strtoul(std::string(p, 4).c_str(), nullptr, 16);
        } else if (p[0] == ' ') {
            data = strtoul(std::string(p + 2, 2).c_str(), nullptr, 16);
        } else {
            data = strtoul(std::string(p, 2).c_str(), nullptr, 16) << 8;
        }
        auto s = Signals::put();
        s->clearMark();
        s->addr = addr;
        s->data = data;
        s->_signals[0] = rw == 'R';   // R/#W
        s->_signals[1] = byte;        // B/#W
        s->_signals[2] = io ? 2 : 8;  // ST
        Cycles::next();
    }
    fclose(f);
    return pc;
}

// findFetch() as on the board, plus what a replay shows.
struct Replay {
    Signals *begin = nullptr;
    const Signals *end = nullptr;
    uint32_t pc = 0;
    unsigned cycles = 0, matched = 0, unexplained = 0, offset = 0;

    void run(const InstZ280::Memory &memory, uint32_t stopPc) {
        pc = stopPc;
        begin = Signals::get();
        end = Signals::put();
        cycles = begin->diff(end);
        const auto at = InstZ280::findFetch(begin, end, memory, pc);
        offset = begin->diff(at);
        matched = unexplained = 0;
        for (unsigned i = 0; i < cycles; ++i) {
            const auto s = begin->next(i);
            if (s->fetch())
                ++matched;
            else if (i >= offset && !s->isByte() && !s->isOperand() &&
                     s->read())
                ++unexplained;
        }
    }

    void print() const {
        printf("start %u, PC=%04X, %u matched, %u unexplained\n", offset, pc,
                matched, unexplained);
        for (unsigned i = 0; i < cycles; ++i) {
            const auto s = begin->next(i);
            printf("%3u %c %c %s=%06X D=%04X%s\n", i,
                    s->fetch()       ? 'I'
                    : s->isByte()    ? 'b'
                    : s->isOperand() ? 'o'
                                     : ' ',
                    s->read() ? 'R' : 'W', s->ioReq() ? "I" : "A", s->addr,
                    s->data, s->byteAccess() ? " byte" : "");
        }
    }
};

Cycles *ring = nullptr;

const auto RINGS = match_harness::loadRings(DIR, "z280");

// The bench rings: where running samples stopped (scripts/record-rings.py).
const auto BENCH = match_harness::loadRings(DIR, "bench_z280");

// Memory as a ring's reads left it, by Z-BUS lane: a word's even byte on
// D15-D8, a byte on its own lane. A bench ring's program wins over them,
// with the breakpoint's RST 38H at its stop, as the board walks it.
struct RingMemory final : InstZ280::Memory {
    std::vector<uint8_t> mem = std::vector<uint8_t>(1 << 16, 0xFF);
    explicit RingMemory(const std::vector<match_harness::Cycle> &cycles) {
        for (const auto &c : cycles) {
            if (c.kind != 'R')
                continue;
            const auto a = c.addr & 0xFFFF;
            if (c.cntl & 0x100) {
                mem[a] = (a & 1) ? c.data : c.data >> 8;
            } else {
                mem[a & ~1] = c.data >> 8;
                mem[a | 1] = c.data;
            }
        }
        match_harness::loadMemory(match_harness::memory,
                [&](uint32_t a, uint8_t b) { mem[a & 0xFFFF] = b; });
    }
    uint16_t read_byte(uint32_t a) const override { return mem[a & 0xFFFF]; }
};

// findFetch() over a recorded run, as the board would: each cycle's mark
// (1 fetch, 2 byte or stale prefetch, 3 data), and the start's index or
// -1. |cntl| is ST, with B/#W above it.
int replay(const std::vector<match_harness::Cycle> &cycles,
        std::vector<int> &marks, uint32_t stop) {
    Cycles::reset();
    for (const auto &c : cycles) {
        auto s = Signals::put();
        s->clearMark();
        s->addr = c.addr;
        s->data = c.data;
        s->_signals[0] = c.kind == 'R' || c.kind == 'r';  // R/#W
        s->_signals[1] = (c.cntl >> 8) & 1;               // B/#W
        s->_signals[2] = c.cntl & 0xFF;                   // ST
        Cycles::next();
    }
    RingMemory memory(cycles);
    if (match_harness::memory)
        memory.mem[stop & 0xFFFF] = InstZ280::RST38;
    const auto begin = Signals::get();
    const auto end = Signals::put();
    MatchWalker::trace = match_harness::tracing();
    const auto at = InstZ280::findFetch(begin, end, memory, stop);
    for (auto i = 0u; i < cycles.size(); ++i) {
        const auto s = begin->next(i);
        marks.push_back(s->fetch()       ? 1
                        : s->isByte()    ? 2
                        : s->isOperand() ? 3
                                         : 0);
    }
    return at == end ? -1 : begin->diff(at);
}

// Replay one dump of |sample| and check its marks against the listing.
void check(const char *sample, const char *dump) {
    Memory memory;
    const auto base = here() + "dumps/" + sample;
    TEST_ASSERT_TRUE_MESSAGE(memory.load(base + ".hex"), "sample HEX");
    const auto starts = listing(base + ".lst");
    TEST_ASSERT_FALSE_MESSAGE(starts.empty(), "sample listing");
    const auto pc = load_dump(here() + "dumps/" + dump);
    Replay replay;
    replay.run(memory, pc);
    TEST_ASSERT_GREATER_THAN_MESSAGE(10, replay.matched, dump);
    // The ring's ends: cut instructions.
    TEST_ASSERT_LESS_OR_EQUAL_MESSAGE(16, replay.unexplained, dump);
    unsigned seen = 0, first = replay.cycles, last = 0;
    for (unsigned i = 0; i < replay.cycles; ++i) {
        const auto s = replay.begin->next(i);
        if (!s->fetch())
            continue;
        last = i;
        ++seen;
        if (seen <= 2)
            continue;  // where a cut ring's leftovers may still sit
        if (first == replay.cycles)
            first = i;
        char msg[80];
        snprintf(msg, sizeof msg,
                "%s: fetch marked inside an instruction at %04X", dump,
                (unsigned)s->addr);
        TEST_ASSERT_TRUE_MESSAGE(starts.count(s->addr & 0xFFFF), msg);
    }
    // Every write between the third and the last instruction belongs
    // to one of them: a failed match must not undo another's marks.
    for (unsigned i = first; i < last; ++i) {
        const auto s = replay.begin->next(i);
        if (!s->write())
            continue;
        char msg[80];
        snprintf(msg, sizeof msg, "%s: write at cycle %u to %04X unexplained",
                dump, i, (unsigned)s->addr);
        TEST_ASSERT_TRUE_MESSAGE(s->isOperand(), msg);
    }
}

}  // namespace

void setUp() {
    ring = new Cycles();
}

void tearDown() {
    delete ring;
    ring = nullptr;
}

void test_arith_to_its_exit() {
    check("arith", "arith.txt");
}

void test_mandelbrot_halted() {
    check("mandelbrot", "mandelbrot.txt");
}

void test_mandelbrot_cut_inside_a_multiply() {
    check("mandelbrot", "mandelbrot-cut.txt");
}

void test_mandelbrot_cut_in_a_multiply() {
    // The tail of the cut multiply decodes as a chain that must go.
    check("mandelbrot", "mandelbrot-cut-in-a-multiply.txt");
}

void test_mandelbrot_cut_at_the_end() {
    // The ring ends inside an instruction, stalled; the one before it
    // keeps its write.
    check("mandelbrot", "mandelbrot-cut-at-end.txt");
}

void test_mandelbrot_cut_in_a_branch() {
    // The ring ends between a conditional jump and its target: taken or
    // not cannot be told, and the chain before it must stand.
    check("mandelbrot", "mandelbrot-cut-in-a-branch.txt");
}

void test_mandelbrot_cut_in_a_divide() {
    // The ring ends inside a divide whose match fails outright; the
    // instruction before it must not be rejected for that.
    check("mandelbrot", "mandelbrot-cut-in-a-divide.txt");
}

void test_echoir_mode_1_interrupts() {
    check("echoir", "echoir.txt");
}

void test_echoitr_mode_0_interrupts() {
    check("echoitr", "echoitr.txt");
}

void test_recorded_runs() {
    match_harness::check(DIR, "z280", RINGS, replay);
}

void test_bench_rings() {
    match_harness::isFetch = [](int mark) { return mark == 1; };
    match_harness::check(DIR, "bench_z280", BENCH, replay);
}

void test_replay_one_dump() {
    // Z280_DUMP=<dump> [Z280_HEX=<sample.hex>]: a diagnostic, not a check.
    const char *dump = getenv("Z280_DUMP");
    if (dump == nullptr)
        TEST_IGNORE_MESSAGE("set Z280_DUMP to replay a dump");
    Memory memory;
    const char *hex = getenv("Z280_HEX");
    TEST_ASSERT_TRUE_MESSAGE(
            memory.load(hex ? hex : here() + "dumps/mandelbrot.hex"),
            "Z280_HEX");
    const auto pc = load_dump(dump);
    Replay replay;
    MatchWalker::trace = getenv("Z280_TRACE") != nullptr;
    replay.run(memory, pc);
    MatchWalker::trace = false;
    replay.print();
}

// Every sequence it walks with is the legend's.
void test_sequences() {
    match_harness::checkSequences(InstZ280::sequences());
    for (const auto seq : InstZ280::interrupts())
        match_harness::checkInterrupt(seq);
}

int main() {
    UNITY_BEGIN();
    RUN_TEST(test_sequences);
    RUN_TEST(test_arith_to_its_exit);
    RUN_TEST(test_mandelbrot_halted);
    RUN_TEST(test_mandelbrot_cut_inside_a_multiply);
    RUN_TEST(test_mandelbrot_cut_in_a_multiply);
    RUN_TEST(test_mandelbrot_cut_at_the_end);
    RUN_TEST(test_mandelbrot_cut_in_a_branch);
    RUN_TEST(test_mandelbrot_cut_in_a_divide);
    RUN_TEST(test_echoir_mode_1_interrupts);
    RUN_TEST(test_echoitr_mode_0_interrupts);
    RUN_TEST(test_recorded_runs);
    RUN_TEST(test_bench_rings);
    RUN_TEST(test_replay_one_dump);
    return UNITY_END();
}
