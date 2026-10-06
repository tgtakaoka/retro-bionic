// Host-side test of the Z280 cycle matcher: `pio test -e native`.
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

#include <unity.h>

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
        while (isxdigit(p[0]) && isxdigit(p[1]) && (p[2] == ' ' || p[2] == '\n')) {
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
    bool verbose = false;

    void run(const InstZ280::Memory &memory, uint32_t stopPc) {
        pc = stopPc;
        begin = Signals::get();
        end = Signals::put();
        cycles = begin->diff(end);
        if (verbose) {
            const auto limit = cycles >= 14 ? 14 : cycles;
            for (unsigned i = 0; i < limit; ++i) {
                bool atPc;
                const auto n = InstZ280::matchAll(begin->next(i), end, memory, pc, atPc);
                printf("start %2u: %2u matched%s\n", i, n, atPc ? ", ends at the PC" : "");
            }
        }
        const auto at = InstZ280::findFetch(begin, end, memory, pc);
        offset = begin->diff(at);
        matched = unexplained = 0;
        for (unsigned i = 0; i < cycles; ++i) {
            const auto s = begin->next(i);
            if (s->fetch())
                ++matched;
            else if (i >= offset && !s->isByte() && !s->isOperand() && s->read())
                ++unexplained;
        }
    }

    void print() const {
        printf("start %u, PC=%04X, %u matched, %u unexplained\n", offset, pc, matched, unexplained);
        for (unsigned i = 0; i < cycles; ++i) {
            const auto s = begin->next(i);
            printf("%3u %c %c %s=%06X D=%04X%s\n", i,
                   s->fetch() ? 'I' : s->isByte() ? 'b' : s->isOperand() ? 'o' : ' ',
                   s->read() ? 'R' : 'W', s->ioReq() ? "I" : "A", s->addr, s->data,
                   s->byteAccess() ? " byte" : "");
        }
    }
};

Cycles *ring = nullptr;

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
        snprintf(msg, sizeof msg, "%s: fetch marked inside an instruction at %04X",
                 dump, (unsigned)s->addr);
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

void test_replay_one_dump() {
    // Z280_DUMP=<dump> [Z280_HEX=<sample.hex>]: a diagnostic, not a check.
    const char *dump = getenv("Z280_DUMP");
    if (dump == nullptr)
        TEST_IGNORE_MESSAGE("set Z280_DUMP to replay a dump");
    Memory memory;
    const char *hex = getenv("Z280_HEX");
    TEST_ASSERT_TRUE_MESSAGE(memory.load(hex ? hex : here() + "dumps/mandelbrot.hex"), "Z280_HEX");
    const auto pc = load_dump(dump);
    Replay replay;
    replay.verbose = true;
    InstZ280::trace = getenv("Z280_TRACE") != nullptr;
    replay.run(memory, pc);
    InstZ280::trace = false;
    replay.print();
}

int main() {
    UNITY_BEGIN();
    RUN_TEST(test_arith_to_its_exit);
    RUN_TEST(test_mandelbrot_halted);
    RUN_TEST(test_mandelbrot_cut_inside_a_multiply);
    RUN_TEST(test_mandelbrot_cut_in_a_multiply);
    RUN_TEST(test_mandelbrot_cut_at_the_end);
    RUN_TEST(test_mandelbrot_cut_in_a_branch);
    RUN_TEST(test_mandelbrot_cut_in_a_divide);
    RUN_TEST(test_echoir_mode_1_interrupts);
    RUN_TEST(test_echoitr_mode_0_interrupts);
    RUN_TEST(test_replay_one_dump);
    return UNITY_END();
}
