// Host-side test of MatchWalker on a toy CPU: `pio test -e native`.
//
// The toy CPU has an 8-bit bus and one page of opcodes, each a sequence of
// match_legend.md; each case below is a ring of its bus cycles and what
// the walk must make of it.

#include <unity.h>

#include <time.h>

#include <stdlib.h>

#include <string>
#include <vector>

#include "match_walker.h"

using namespace debugger;

namespace {

struct ToySignals final : SignalsBase<ToySignals> {
    void set(char kind, uint16_t a, uint16_t d) {
        _signals[0] = kind;
        addr = a;
        data = d;
        _signals[1] = _signals[2] = 0;
    }
    char kind() const { return _signals[0]; }
    uint8_t role() const { return _signals[1]; }
    uint8_t span() const { return _signals[2]; }
    void mark(uint8_t role, uint8_t span) {
        _signals[1] = role;
        _signals[2] = span;
    }
};

// Opcodes: the sequence, and the target's or effective address's operand.
struct Opcode {
    const char *seq;
    bool target;  // the operand bytes are the target
    bool ea;      // the operand bytes are the effective address
};

const Opcode OPCODES[] = {
        {"1N", false, false},        // 00 NOP
        {"12N", false, false},       // 01 LD A,#n
        {"122AN", false, true},      // 02 LD A,(nn)
        {"122J", true, false},       // 03 JP nn
        {"1RrP", false, false},      // 04 RET: pops the PC
        {"12J@12N", true, false},    // 05 BEQ d: taken or not
        {"1{R+1}N", false, false},   // 06 SCAN: reads up memory
        {"1WwN", false, false},      // 07 PUSH: high below low
        {"1-N", false, false},       // 08 WAIT: a cycle without address
        {"122BN", false, true},      // 09 ST (nn),A
        {"1NxR", false, false},      // 0A PUL: fetches the next ahead
        {"1nN", false, false},       // 0B NOP2: reads its next twice
        {"1R", false, false},        // 0C POP: names no next fetch
        {"12n?@12N", false, false},  // 0D JR cc: reads its next, then anywhere
        {"1{R+1W+1}N", false, false},  // 0E COPY: up memory to up memory
        {"1~RN", false, false},        // 0F LDQ: reads past a queue
        {"12~J", true, false},         // 10 JRQ d: jumps past a queue
        {"1RrN", false, false},        // 11 LDW: reads a word
        {"1WwWwN", false, false},      // 12 PUSH2: pushes two words
        {"1RN", false, false},         // 13 LDB: reads a byte
        {"122~[WwJ]", true, false},    // 14 CALLQ nn: pushes, jumps, any order
        {"1~[RW]N", false, false},     // 15 XCHG: reads and writes, any order
        {"1~{h}N", false, false},      // 16 HALT: halts until an interrupt
        {"122~[RJ]", true, false},     // 17 JPR nn: reads, jumps, any order
};
constexpr uint8_t OPCODE_COUNT = sizeof(OPCODES) / sizeof(OPCODES[0]);

struct Toy : MatchWalker::Arch {
    explicit Toy(const MatchWalker::Traits &t = MatchWalker::Traits{})
        : Arch(t) {}
    const char *interrupt = nullptr;
    std::vector<uint8_t> mem = std::vector<uint8_t>(0x10000, 0xFF);

    MatchWalker::Kind cycleKind(const SignalsImpl *s) const override {
        const auto k = static_cast<const ToySignals *>(s)->kind();
        return k == 'R' || k == 'Q'   ? MatchWalker::K_READ
               : k == 'W' || k == 'U' ? MatchWalker::K_WRITE
               : k == 'H'             ? MatchWalker::K_HALT
                                      : MatchWalker::K_NONE;
    }
    uint_fast8_t fetchBytes(const SignalsImpl *s) const override {
        return dataBytes(s);
    }
    uint_fast8_t dataBytes(const SignalsImpl *s) const override {
        const auto k = static_cast<const ToySignals *>(s)->kind();
        return k == 'Q' || k == 'U' ? 2 : 1;
    }
    uint8_t dataByte(const SignalsImpl *s, uint_fast8_t k) const override {
        return k ? s->data >> 8 : s->data & 0xFF;
    }
    // an MMU that maps 4K pages, if set
    bool mmu = false;
    bool sameAddress(uint32_t bus, uint32_t addr) const override {
        return mmu ? (bus & 0xFFF) == (addr & 0xFFF) : bus == addr;
    }

    bool decode(uint32_t pc, MatchWalker::Decoded &inst) const override {
        const auto opc = mem[pc & 0xFFFF];
        if (opc >= OPCODE_COUNT)
            return false;
        const auto &o = OPCODES[opc];
        inst.pc = pc;
        inst.seq = o.seq;
        inst.length = MatchWalker::instructionBytes(o.seq);
        inst.hasTarget = inst.hasEa = false;
        const uint16_t operand = mem[(pc + 1) & 0xFFFF] | mem[(pc + 2) & 0xFFFF]
                                                                  << 8;
        if (o.target && inst.length == 3) {
            inst.hasTarget = true;
            inst.target = operand;
        } else if (o.target) {  // a displacement
            inst.hasTarget = true;
            inst.target = (pc + 2 + int8_t(mem[(pc + 1) & 0xFFFF])) & 0xFFFF;
        }
        if (o.ea) {
            inst.hasEa = true;
            inst.ea = operand;
        }
        return true;
    }

    bool isVectorTable(uint32_t addr) const override { return addr >= 0xFFF8; }
    bool isDummy(const SignalsImpl *s) const override {
        return s->addr == 0xFFFF;
    }
    const char *interruptSequence() const override { return interrupt; }
    const char *resume = nullptr;
    const char *resumeSequence() const override { return resume; }

    void markCycle(SignalsImpl *s, MatchWalker::Role role,
            uint_fast8_t span) const override {
        static_cast<ToySignals *>(s)->mark(role, span);
    }
};

struct Cycle {
    char kind;  // R, W, Q (a word read), U (a word write) or - (no address)
    uint16_t addr;
    uint16_t data;  // a word's low byte first
};

Toy plain;
Toy *arch = &plain;
#define toy (*arch)
MatchWalker walker;

// Code to load: where, and its bytes.
struct Code {
    uint16_t org;
    std::vector<uint8_t> bytes;
};

// Loads |code|, feeds |cycles| and walks them; the marks as one character
// per cycle: F<span> fetch, b byte, d data, . none.
std::string walk(const std::vector<Code> &code,
        const std::vector<Cycle> &cycles, bool &walked,
        uint32_t stop = MatchWalker::NO_STOP) {
    toy.mem.assign(0x10000, 0xFF);
    for (const auto &c : code) {
        for (auto i = 0u; i < c.bytes.size(); ++i)
            toy.mem[(c.org + i) & 0xFFFF] = c.bytes[i];
    }
    Cycles::reset();
    for (const auto &c : cycles) {
        ToySignals::put()->set(c.kind, c.addr, c.data);
        Cycles::next();
    }
    const auto begin = ToySignals::get();
    walked = walker.walk(toy, begin, ToySignals::put(), stop);
    std::string marks;
    for (auto i = 0u; i < cycles.size(); ++i) {
        const auto s = begin->next(i);
        switch (s->role()) {
        case MatchWalker::R_FETCH:
            marks += "F" + std::to_string(s->span());
            break;
        case MatchWalker::R_BYTE:
            marks += "b";
            break;
        case MatchWalker::R_DATA:
            marks += "d";
            break;
        default:
            marks += ".";
            break;
        }
        marks += ' ';
    }
    marks.pop_back();
    return marks;
}

Cycles *ring;

}  // namespace

void setUp() {
    ring = new Cycles();
}

void tearDown() {
    delete ring;
}

// NOP, LD A,#n, LD A,(nn): each fetch marked with its cycles.
void test_straight_line() {
    bool walked;
    const auto marks = walk({{0x100, {0x00, 0x01, 0x12, 0x02, 0x00, 0x20}}},
            {{'R', 0x100, 0x00}, {'R', 0x101, 0x01}, {'R', 0x102, 0x12},
                    {'R', 0x103, 0x02}, {'R', 0x104, 0x00}, {'R', 0x105, 0x20},
                    {'R', 0x2000, 0x55}},
            walked);
    TEST_ASSERT_TRUE(walked);
    TEST_ASSERT_EQUAL_STRING("F1 F2 b F4 b b d", marks.c_str());
    TEST_ASSERT_EQUAL_UINT(0, walker.start());
    TEST_ASSERT_EQUAL_UINT(3, walker.steps());
    TEST_ASSERT_EQUAL_HEX32(0x103, walker.addr(2));
}

// BEQ taken goes to its target, not taken to the next instruction.
void test_conditional() {
    bool walked;
    auto marks = walk({{0x100, {0x05, 0x10}}, {0x112, {0x00}}},
            {{'R', 0x100, 0x05}, {'R', 0x101, 0x10}, {'R', 0x112, 0x00}},
            walked);
    TEST_ASSERT_TRUE(walked);
    TEST_ASSERT_EQUAL_STRING("F2 b F1", marks.c_str());
    TEST_ASSERT_EQUAL_HEX32(0x112, walker.addr(1));
    marks = walk({{0x100, {0x05, 0x10, 0x00}}},
            {{'R', 0x100, 0x05}, {'R', 0x101, 0x10}, {'R', 0x102, 0x00}},
            walked);
    TEST_ASSERT_TRUE(walked);
    TEST_ASSERT_EQUAL_HEX32(0x102, walker.addr(1));
}

// The taken branch's target fits its fetch but fails after: the walk goes
// back and takes the branch not taken.
void test_backtrack_across_instructions() {
    bool walked;
    // BEQ +0: taken and not taken both reach the WAIT at 102
    const auto marks = walk({{0x100, {0x05, 0x00, 0x08, 0x00}}},
            {{'R', 0x100, 0x05}, {'R', 0x101, 0x00}, {'R', 0x102, 0x08},
                    {'-', 0, 0}, {'R', 0x103, 0xFF}},
            walked);
    TEST_ASSERT_TRUE(walked);
    TEST_ASSERT_EQUAL_STRING("F2 b F2 . F1", marks.c_str());
    // BEQ -4: taken would fetch 0FE next, but 102 comes; not taken goes
    // on there
    const auto back = walk({{0x100, {0x05, 0xFC, 0x00, 0x00}}, {0x0FE, {0x01}}},
            {{'R', 0x100, 0x05}, {'R', 0x101, 0xFC}, {'R', 0x102, 0x00},
                    {'R', 0x103, 0xFF}},
            walked);
    TEST_ASSERT_TRUE(walked);
    TEST_ASSERT_EQUAL_STRING("F2 b F1 F1", back.c_str());
    TEST_ASSERT_EQUAL_HEX32(0x102, walker.addr(1));
}

// SCAN reads up memory one byte at a time, as many as there are.
void test_repeat_group() {
    bool walked;
    const auto marks = walk({{0x100, {0x06, 0x00}}},
            {{'R', 0x100, 0x06}, {'R', 0x3000, 1}, {'R', 0x3001, 2},
                    {'R', 0x3002, 3}, {'R', 0x101, 0x00}},
            walked);
    TEST_ASSERT_TRUE(walked);
    TEST_ASSERT_EQUAL_STRING("F4 d d d F1", marks.c_str());
}

// RET goes where the two bytes it popped say.
void test_popped_value() {
    bool walked;
    const auto marks = walk({{0x100, {0x04}}, {0x1234, {0x00}}},
            {{'R', 0x100, 0x04}, {'R', 0xFF0, 0x34}, {'R', 0xFF1, 0x12},
                    {'R', 0x1234, 0x00}},
            walked);
    TEST_ASSERT_TRUE(walked);
    TEST_ASSERT_EQUAL_STRING("F3 d d F1", marks.c_str());
    TEST_ASSERT_EQUAL_HEX32(0x1234, walker.addr(1));
}

// A ring ends inside its last instruction where a halt cut it.
void test_ring_ends_inside() {
    bool walked;
    auto marks = walk({{0x100, {0x07}}},
            {{'R', 0x100, 0x07}, {'W', 0xFFE, 0x12}, {'W', 0xFFF, 0x34}},
            walked);
    TEST_ASSERT_TRUE(walked);
    TEST_ASSERT_EQUAL_STRING("F3 d d", marks.c_str());
    marks = walk({{0x100, {0x02, 0x00, 0x20}}},
            {{'R', 0x100, 0x02}, {'R', 0x101, 0x00}, {'R', 0x102, 0x20}},
            walked);
    TEST_ASSERT_TRUE(walked);
    TEST_ASSERT_EQUAL_STRING("F3 b b", marks.c_str());
}

// With the stop known, the walk must end there.
void test_stop() {
    bool walked;
    walk({{0x100, {0x00, 0x00}}}, {{'R', 0x100, 0x00}, {'R', 0x101, 0x00}},
            walked, 0x102);
    TEST_ASSERT_TRUE(walked);
    // where no walk reaches the stop, one that reaches the ring's end
    walk({{0x100, {0x00, 0x00}}}, {{'R', 0x100, 0x00}, {'R', 0x101, 0x00}},
            walked, 0x200);
    TEST_ASSERT_TRUE(walked);
}

// A ring that starts with what no instruction explains: the earliest
// start that walks.
void test_earliest_start() {
    bool walked;
    const auto marks = walk({{0x100, {0x00, 0x00}}, {0x0F0, {0x0A}}},
            {{'W', 0x2000, 0x00}, {'R', 0x0F0, 0x0A}, {'R', 0x100, 0x00},
                    {'R', 0x101, 0x00}},
            walked);
    TEST_ASSERT_TRUE(walked);
    TEST_ASSERT_EQUAL_UINT(2, walker.start());
    TEST_ASSERT_EQUAL_STRING(". . F1 F1", marks.c_str());
}

// No start walks: nothing walked, nothing marked.
void test_no_walk() {
    bool walked;
    const auto marks = walk({{0x100, {0x30, 0x30}}},
            {{'R', 0x100, 0x30}, {'R', 0x101, 0x30}}, walked);
    TEST_ASSERT_FALSE(walked);
    TEST_ASSERT_EQUAL_STRING(". .", marks.c_str());
    TEST_ASSERT_EQUAL_UINT(0, walker.steps());
}

// ST (nn),A writes its effective address and nowhere else.
void test_effective_address() {
    bool walked;
    walk({{0x100, {0x09, 0x00, 0x20, 0x00}}},
            {{'R', 0x100, 0x09}, {'R', 0x101, 0x00}, {'R', 0x102, 0x20},
                    {'W', 0x2000, 0x55}, {'R', 0x103, 0x00}},
            walked);
    TEST_ASSERT_TRUE(walked);
    TEST_ASSERT_EQUAL_UINT(0, walker.start());
    walk({{0x100, {0x09, 0x00, 0x20, 0x00}}},
            {{'R', 0x100, 0x09}, {'R', 0x101, 0x00}, {'R', 0x102, 0x20},
                    {'W', 0x2001, 0x55}, {'R', 0x103, 0x00}},
            walked);
    TEST_ASSERT_NOT_EQUAL(0, walker.start());
}

// PUL fetches the next instruction ahead, before its own data: the next
// one's 1 takes no cycle, and its fetch is marked there.
void test_hand_over() {
    bool walked;
    const auto marks = walk({{0x100, {0x0A, 0x00, 0x00}}},
            {{'R', 0x100, 0x0A}, {'R', 0x101, 0x00}, {'R', 0xFFFF, 0},
                    {'R', 0x2000, 0x55}, {'R', 0x102, 0x00}},
            walked);
    TEST_ASSERT_TRUE(walked);
    // the NOP at 101 fetched at cycle 1 and took no cycle of its own
    TEST_ASSERT_EQUAL_STRING("F4 F0 d d F1", marks.c_str());
    TEST_ASSERT_EQUAL_HEX32(0x101, walker.addr(1));
}

// NOP2 reads its next address twice: the first doesn't advance.
void test_dummy_next_read() {
    bool walked;
    const auto marks = walk({{0x100, {0x0B, 0x00}}},
            {{'R', 0x100, 0x0B}, {'R', 0x101, 0x00}, {'R', 0x101, 0x00}},
            walked);
    TEST_ASSERT_TRUE(walked);
    TEST_ASSERT_EQUAL_STRING("F2 b F1", marks.c_str());
}

// POP names no next fetch: the next instruction is wherever it reads.
void test_no_next_fetch_named() {
    bool walked;
    const auto marks = walk({{0x100, {0x0C}}, {0x300, {0x00}}},
            {{'R', 0x100, 0x0C}, {'R', 0xFF0, 0x12}, {'R', 0x300, 0x00}},
            walked);
    TEST_ASSERT_TRUE(walked);
    TEST_ASSERT_EQUAL_STRING("F2 d F1", marks.c_str());
    TEST_ASSERT_EQUAL_HEX32(0x300, walker.addr(1));
}

// Where no instruction fits, an interrupt: no fetch of its own marked.
void test_interrupt() {
    bool walked;
    Toy intr;
    intr.interrupt = "XwWVrP";
    arch = &intr;
    const auto marks = walk({{0x100, {0x01, 0x12}}, {0x400, {0x00}}},
            {{'R', 0x100, 0x01}, {'W', 0xFFF, 0x00}, {'W', 0xFFE, 0x01},
                    {'R', 0xFFF8, 0x00}, {'R', 0xFFF9, 0x04},
                    {'R', 0x400, 0x00}},
            walked);
    arch = &plain;
    TEST_ASSERT_TRUE(walked);
    TEST_ASSERT_EQUAL_STRING("d d d d d F1", marks.c_str());
    TEST_ASSERT_EQUAL_HEX32(0x400, walker.addr(1));
}

// An interrupt the ring cuts before its vector read is only any reads:
// it doesn't make a start that fails walk.
void test_interrupt_cut_before_vector() {
    bool walked;
    Toy intr;
    intr.interrupt = "XXXXV?";
    arch = &intr;
    const auto marks = walk({{0x100, {0x0B, 0x00}}, {0x200, {0x00, 0x00}}},
            {{'R', 0x100, 0x0B}, {'R', 0x101, 0x00}, {'R', 0x200, 0x00},
                    {'R', 0x201, 0x00}},
            walked);
    arch = &plain;
    TEST_ASSERT_TRUE(walked);
    TEST_ASSERT_EQUAL_UINT(2, walker.start());
    TEST_ASSERT_EQUAL_STRING(". . F1 F1", marks.c_str());
}

// In a group, a stepped token's step alone decides its address: COPY's
// writes go up, where a W's own would go down.
void test_group_step_decides() {
    bool walked;
    const auto marks = walk({{0x100, {0x0E, 0x00}}},
            {{'R', 0x100, 0x0E}, {'R', 0x2000, 1}, {'W', 0x3000, 1},
                    {'R', 0x2001, 2}, {'W', 0x3001, 2}, {'R', 0x101, 0x00}},
            walked);
    TEST_ASSERT_TRUE(walked);
    TEST_ASSERT_EQUAL_STRING("F5 d d d d F1", marks.c_str());
}

// A prefetch queue of 4.
const MatchWalker::Traits QUEUE{.queue = 4};

// Past LDQ's ~, the queue fetches the next four NOPs ahead, around its
// read: they take no cycle of their own, and the fifth's is the next fetch.
void test_queue_fetches_ahead() {
    bool walked;
    Toy queue(QUEUE);
    arch = &queue;
    const auto marks = walk({{0x100, {0x0F, 0x00, 0x00, 0x00, 0x00, 0x00}}},
            {{'R', 0x100, 0x0F}, {'R', 0x101, 0x00}, {'R', 0x102, 0x00},
                    {'R', 0x2000, 0x55}, {'R', 0x103, 0x00}, {'R', 0x104, 0x00},
                    {'R', 0x105, 0x00}},
            walked);
    arch = &plain;
    TEST_ASSERT_TRUE(walked);
    TEST_ASSERT_EQUAL_STRING("F6 F0 F0 d F0 F0 F1", marks.c_str());
    TEST_ASSERT_EQUAL_UINT(6, walker.steps());
}

// The queue holds as many as it can: a third fetch ahead isn't one.
void test_queue_capacity() {
    bool walked;
    const std::vector<Code> code = {{0x100, {0x0F, 0x00, 0x00, 0x00}}};
    const std::vector<Cycle> cycles = {{'R', 0x100, 0x0F}, {'R', 0x101, 0x00},
            {'R', 0x102, 0x00}, {'R', 0x103, 0x00}, {'R', 0x2000, 0x55}};
    Toy roomy(QUEUE);
    arch = &roomy;
    walk(code, cycles, walked);
    TEST_ASSERT_TRUE(walked);
    Toy small(MatchWalker::Traits{.queue = 2});
    arch = &small;
    walk(code, cycles, walked);
    arch = &plain;
    TEST_ASSERT_FALSE(walked);
}

// A jump loses what the queue fetched ahead: no fetch marked there.
void test_queue_flushed() {
    bool walked;
    Toy queue(QUEUE);
    arch = &queue;
    const auto marks = walk({{0x100, {0x10, 0x10}}, {0x112, {0x00, 0x00}}},
            {{'R', 0x100, 0x10}, {'R', 0x101, 0x10}, {'R', 0x102, 0xFF},
                    {'R', 0x103, 0xFF}, {'R', 0x112, 0x00}, {'R', 0x113, 0x00}},
            walked);
    arch = &plain;
    TEST_ASSERT_TRUE(walked);
    TEST_ASSERT_EQUAL_STRING("F4 b b b F1 F1", marks.c_str());
}

// An interrupt's ~ takes the opcode it aborted as the queue's: no fetch
// marked there.
void test_queue_interrupt() {
    bool walked;
    Toy queue(QUEUE);
    queue.interrupt = "~VrWwP";
    arch = &queue;
    const auto marks = walk({{0x100, {0x00, 0x01, 0x12}},
                                    {0xFFF8, {0x00, 0x04}}, {0x400, {0x00}}},
            {{'R', 0x100, 0x00}, {'R', 0x101, 0x01}, {'R', 0x102, 0x12},
                    {'R', 0xFFF8, 0x00}, {'R', 0xFFF9, 0x04},
                    {'W', 0x0FFF, 0x01}, {'W', 0x1000, 0x01},
                    {'R', 0x400, 0x00}},
            walked);
    arch = &plain;
    TEST_ASSERT_TRUE(walked);
    TEST_ASSERT_EQUAL_STRING("F1 b b d d d d F1", marks.c_str());
}

// A word cycle meets a word's two tokens, or a byte's one.
void test_word_cycles() {
    bool walked;
    auto marks = walk({{0x100, {0x11, 0x00}}},
            {{'R', 0x100, 0x11}, {'Q', 0x2000, 0x1234}, {'R', 0x101, 0x00}},
            walked);
    TEST_ASSERT_TRUE(walked);
    TEST_ASSERT_EQUAL_STRING("F2 d F1", marks.c_str());
    marks = walk({{0x100, {0x13, 0x00}}},
            {{'R', 0x100, 0x13}, {'Q', 0x2000, 0x0055}, {'R', 0x101, 0x00}},
            walked);
    TEST_ASSERT_TRUE(walked);
    TEST_ASSERT_EQUAL_STRING("F2 d F1", marks.c_str());
    // a word below the one before: below by its width
    marks = walk({{0x100, {0x12, 0x00}}},
            {{'R', 0x100, 0x12}, {'U', 0x0FFE, 0x0101}, {'U', 0x0FFC, 0},
                    {'R', 0x101, 0x00}},
            walked);
    TEST_ASSERT_TRUE(walked);
    TEST_ASSERT_EQUAL_STRING("F3 d d F1", marks.c_str());
}

// A stall fetches again the byte the stream fetched last.
void test_refetch() {
    bool walked;
    const std::vector<Code> code = {{0x100, {0x01, 0x12, 0x00}}};
    const std::vector<Cycle> cycles = {{'R', 0x100, 0x01}, {'R', 0x100, 0x01},
            {'R', 0x101, 0x12}, {'R', 0x102, 0x00}};
    auto marks = walk(code, cycles, walked);
    TEST_ASSERT_EQUAL_UINT(1, walker.start());
    Toy stalls(MatchWalker::Traits{.refetch = 1});
    arch = &stalls;
    marks = walk(code, cycles, walked);
    arch = &plain;
    TEST_ASSERT_TRUE(walked);
    TEST_ASSERT_EQUAL_UINT(0, walker.start());
    TEST_ASSERT_EQUAL_STRING("F3 b b F1", marks.c_str());
}

// The ring may begin inside an instruction whose first bytes were fetched
// before it: no fetch marked for it, and a next fetch must confirm it.
void test_cut_start() {
    bool walked;
    Toy cut(MatchWalker::Traits{.cutStart = 2});
    arch = &cut;
    const auto marks = walk({{0x100, {0x01, 0x30, 0x00, 0x00}}},
            {{'R', 0x101, 0x30}, {'R', 0x102, 0x00}, {'R', 0x103, 0x00}},
            walked);
    arch = &plain;
    TEST_ASSERT_TRUE(walked);
    TEST_ASSERT_EQUAL_STRING("b F1 F1", marks.c_str());
    TEST_ASSERT_EQUAL_HEX32(0x100, walker.addr(0));
}

// Where an MMU maps the code, a fetch compares only the page offset, but
// an instruction's first fetch tells where its page is: a data read at the
// same offset in another isn't a refetch.
void test_mmu() {
    bool walked;
    Toy mapped(MatchWalker::Traits{.refetch = 1});
    mapped.mmu = true;
    arch = &mapped;
    const auto marks = walk({{0x5100, {0x13, 0x00}}},
            {{'R', 0x5100, 0x13}, {'R', 0x2100, 0x55}, {'R', 0x5101, 0x00}},
            walked);
    arch = &plain;
    TEST_ASSERT_TRUE(walked);
    TEST_ASSERT_EQUAL_STRING("F2 d F1", marks.c_str());
}

// In [ ], each kind's cycles in order, but the kinds and the transfer in
// any, the queue fetching ahead between them.
void test_unordered() {
    bool walked;
    Toy queue(QUEUE);
    arch = &queue;
    const std::vector<Code> code = {
            {0x100, {0x14, 0x00, 0x02}}, {0x200, {0x00, 0x00}}};
    // the push, then the target's fetch
    auto marks = walk(code,
            {{'R', 0x100, 0x14}, {'R', 0x101, 0x00}, {'R', 0x102, 0x02},
                    {'R', 0x103, 0xFF}, {'W', 0x0FFE, 0x03}, {'W', 0x0FFF, 1},
                    {'R', 0x200, 0x00}, {'R', 0x201, 0x00}},
            walked);
    TEST_ASSERT_TRUE(walked);
    TEST_ASSERT_EQUAL_STRING("F7 b b b d d F0 F1", marks.c_str());
    // the target's fetch before the push
    marks = walk(code,
            {{'R', 0x100, 0x14}, {'R', 0x101, 0x00}, {'R', 0x102, 0x02},
                    {'R', 0x200, 0x00}, {'W', 0x0FFE, 0x03}, {'W', 0x0FFF, 1},
                    {'R', 0x201, 0x00}},
            walked);
    TEST_ASSERT_TRUE(walked);
    TEST_ASSERT_EQUAL_STRING("F6 b b F0 d d F1", marks.c_str());
    // a write before the read it goes with
    marks = walk({{0x100, {0x15, 0x00}}},
            {{'R', 0x100, 0x15}, {'W', 0x2000, 1}, {'R', 0x3000, 2},
                    {'R', 0x101, 0x00}},
            walked);
    arch = &plain;
    TEST_ASSERT_TRUE(walked);
    TEST_ASSERT_EQUAL_STRING("F4 d d b", marks.c_str());
}

// A word fetch brings two instruction bytes: the second, the queue's.
void test_word_fetch() {
    bool walked;
    Toy queue(QUEUE);
    arch = &queue;
    walk({{0x100, {0x00, 0x00, 0x00, 0x00}}},
            {{'Q', 0x100, 0x0000}, {'Q', 0x102, 0x0000}}, walked);
    arch = &plain;
    TEST_ASSERT_TRUE(walked);
    // the last, only fetched when the ring ends, isn't walked
    TEST_ASSERT_EQUAL_UINT(3, walker.steps());
    TEST_ASSERT_EQUAL_HEX32(0x101, walker.addr(1));
}

// The instructions the queue holds ran to the stop with no cycles of
// their own: walked, up to it.
void test_ran_to_stop() {
    bool walked;
    Toy queue(QUEUE);
    arch = &queue;
    const auto marks = walk({{0x100, {0x0F, 0x00, 0x00}}},
            {{'R', 0x100, 0x0F}, {'R', 0x101, 0x00}, {'R', 0x2000, 0x55}},
            walked, 0x102);
    arch = &plain;
    TEST_ASSERT_TRUE(walked);
    TEST_ASSERT_EQUAL_UINT(2, walker.steps());
    TEST_ASSERT_EQUAL_HEX32(0x101, walker.addr(1));
    TEST_ASSERT_EQUAL_STRING("F3 F0 d", marks.c_str());
}

// Where the ring's first fetches may be the queue's, its first
// instructions count from one with evidence.
void test_lead_unproven() {
    bool walked;
    Toy lead(MatchWalker::Traits{.queue = 4, .leadUnproven = true});
    arch = &lead;
    walk({{0x100, {0x00, 0x00, 0x13, 0x00}}},
            {{'R', 0x100, 0x00}, {'R', 0x101, 0x00}, {'R', 0x102, 0x13},
                    {'R', 0x2000, 0x55}, {'R', 0x103, 0x00}},
            walked);
    arch = &plain;
    TEST_ASSERT_TRUE(walked);
    TEST_ASSERT_EQUAL_HEX32(0x102, walker.addr(0));
}

// HALT's halt cycles, then on.
void test_halt() {
    bool walked;
    const auto marks = walk({{0x100, {0x16, 0x00}}},
            {{'R', 0x100, 0x16}, {'H', 0, 0}, {'H', 0, 0}, {'R', 0x101, 0x00}},
            walked);
    TEST_ASSERT_TRUE(walked);
    // the next, fetched ahead as the ring ends, isn't walked: the halt's
    // cycle, as the queue's are
    TEST_ASSERT_EQUAL_STRING("F4 d d b", marks.c_str());
}

// A read at a [ ] transfer's target a data token could take too: the
// transfer, or where the group can't end that way, the data.
void test_unordered_read_at_target() {
    bool walked;
    Toy queue(QUEUE);
    queue.interrupt = "~[Ww?]";
    arch = &queue;
    const std::vector<Code> code = {
            {0x100, {0x17, 0x00, 0x02}}, {0x200, {0x00, 0x00}}, {0x400, {0x00}}};
    // the read, then the target's fetch
    auto marks = walk(code,
            {{'R', 0x100, 0x17}, {'R', 0x101, 0x00}, {'R', 0x102, 0x02},
                    {'R', 0x200, 0x00}, {'R', 0x200, 0x00}, {'R', 0x201, 0x00}},
            walked);
    TEST_ASSERT_TRUE(walked);
    TEST_ASSERT_EQUAL_STRING("F5 b b F0 d F1", marks.c_str());
    // the read at the target, then an interrupt before its fetch
    marks = walk(code,
            {{'R', 0x100, 0x17}, {'R', 0x101, 0x00}, {'R', 0x102, 0x02},
                    {'R', 0x200, 0x00}, {'W', 0x0FFE, 0x00},
                    {'W', 0x0FFF, 0x02}, {'R', 0x400, 0x00}},
            walked);
    arch = &plain;
    TEST_ASSERT_TRUE(walked);
    TEST_ASSERT_EQUAL_STRING("F4 b b d d d b", marks.c_str());
    TEST_ASSERT_EQUAL_UINT(2, walker.steps());
    TEST_ASSERT_TRUE(walker.interrupt(1));
    TEST_ASSERT_EQUAL_HEX32(0x200, walker.addr(1));
}

// An interrupt taken before a transfer's fetch: it pushes the target.
void test_interrupt_before_target() {
    bool walked;
    Toy queue(QUEUE);
    queue.interrupt = "~[Ww?]";
    arch = &queue;
    walk({{0x100, {0x14, 0x00, 0x02}}, {0x200, {0x00}}, {0x400, {0x00}}},
            {{'R', 0x100, 0x14}, {'R', 0x101, 0x00}, {'R', 0x102, 0x02},
                    {'W', 0x0FFE, 0x03}, {'W', 0x0FFF, 0x01},
                    {'W', 0x0FFC, 0x00}, {'W', 0x0FFD, 0x02},
                    {'R', 0x400, 0x00}},
            walked);
    arch = &plain;
    TEST_ASSERT_TRUE(walked);
    TEST_ASSERT_EQUAL_UINT(2, walker.steps());
    TEST_ASSERT_TRUE(walker.interrupt(1));
    TEST_ASSERT_EQUAL_HEX32(0x200, walker.addr(1));
}

// Addresses that step wrap at the CPU's mask.
void test_wraps_at_mask() {
    bool walked;
    const auto marks = walk({{0x100, {0x07, 0x00}}},
            {{'R', 0x100, 0x07}, {'W', 0xFFFF, 0x01}, {'W', 0x0000, 0x02},
                    {'R', 0x101, 0x00}},
            walked);
    TEST_ASSERT_TRUE(walked);
    TEST_ASSERT_EQUAL_STRING("F3 d d F1", marks.c_str());
}

// A walk nothing confirms goes back to try the next alternative: JR's
// taken one is cut, its next fetch unseen; the not taken one sees it.
void test_unconfirmed_backtracks() {
    bool walked;
    const auto marks = walk({{0x100, {0x0D, 0x00, 0x00}}},
            {{'W', 0x2000, 0}, {'R', 0x100, 0x0D}, {'R', 0x101, 0x00},
                    {'R', 0x102, 0x00}},
            walked);
    TEST_ASSERT_TRUE(walked);
    TEST_ASSERT_EQUAL_UINT(1, walker.start());
    TEST_ASSERT_EQUAL_STRING(". F2 b F1", marks.c_str());
}

// Where the ring's last cycle is a next fetch, it doesn't cut an
// instruction that took a cycle of its own past its fetch.
void test_last_is_next_fetch_cut() {
    bool walked;
    Toy last(MatchWalker::Traits{.lastIsNextFetch = true});
    arch = &last;
    // JR's taken alternative would end inside it, after its n
    auto marks = walk({{0x100, {0x0D, 0x00}}},
            {{'R', 0x100, 0x0D}, {'R', 0x101, 0x00}, {'R', 0x102, 0x00}},
            walked);
    TEST_ASSERT_TRUE(walked);
    TEST_ASSERT_EQUAL_STRING("F2 b .", marks.c_str());
    // PUL fetched the next ahead: the ring may end after its own cycles,
    // and the next, with none walked, has no fetch marked
    marks = walk({{0x100, {0x0A, 0x00}}},
            {{'R', 0x100, 0x0A}, {'R', 0x101, 0x00}, {'R', 0xFFFF, 0},
                    {'R', 0x2000, 0x55}},
            walked);
    arch = &plain;
    TEST_ASSERT_TRUE(walked);
    TEST_ASSERT_EQUAL_STRING("F4 b d d", marks.c_str());
}

// With the ring's last cycle only the next fetch, no instruction is
// walked from it, and a walk needs a next fetch it saw.
void test_last_is_next_fetch() {
    bool walked;
    Toy last(MatchWalker::Traits{.lastIsNextFetch = true});
    arch = &last;
    auto marks = walk({{0x100, {0x00, 0x00}}},
            {{'R', 0x100, 0x00}, {'R', 0x101, 0x00}}, walked);
    TEST_ASSERT_TRUE(walked);
    TEST_ASSERT_EQUAL_STRING("F1 .", marks.c_str());
    TEST_ASSERT_EQUAL_UINT(1, walker.steps());
    // LD A,(nn) cut before its read: nothing confirms it, and its operand
    // bytes are no instructions
    walk({{0x100, {0x02, 0x30, 0x30}}},
            {{'R', 0x100, 0x02}, {'R', 0x101, 0x30}, {'R', 0x102, 0x30}},
            walked);
    arch = &plain;
    TEST_ASSERT_FALSE(walked);
}

// A start past the first cycles a CPU allows isn't tried.
void test_max_start() {
    bool walked;
    Toy few(MatchWalker::Traits{.maxStart = 1});
    arch = &few;
    walk({{0x100, {0x00, 0x00}}},
            {{'W', 0x2000, 0}, {'R', 0x100, 0x00}, {'R', 0x101, 0x00}}, walked);
    arch = &plain;
    TEST_ASSERT_FALSE(walked);
}

// A ring that backtracks without end: at each cycle a NOP or an interrupt
// that takes any read, and a stop none reaches. The budget gives it up,
// and the walk to the ring's end takes over.
void test_budget() {
    bool walked;
    Toy any;
    any.interrupt = "X?";
    arch = &any;
    std::vector<Code> code;
    std::vector<Cycle> cycles;
    for (uint16_t pc = 0x100; pc < 0x100 + 120; ++pc) {
        code.push_back({pc, {0x00}});  // NOP
        cycles.push_back({'R', pc, 0x00});
    }
    const auto from = clock();
    walk(code, cycles, walked, 0x8000);
    const auto ms = (clock() - from) * 1000 / CLOCKS_PER_SEC;
    arch = &plain;
    printf("a walk out of budget took %ld ms on the host\n", long(ms));
    TEST_ASSERT_TRUE(walked);
}

// A ring begun inside a wait: cycles of no address, the vector, and the
// handler's fetch, then on from there.
void test_resume() {
    bool walked;
    Toy waits;
    waits.resume = "{-}VrP";
    arch = &waits;
    const auto marks = walk({{0x200, {0x00, 0x00}}},
            {{'-', 0, 0}, {'-', 0, 0}, {'-', 0, 0}, {'R', 0xFFF8, 0x00},
                    {'R', 0xFFF9, 0x02}, {'R', 0x200, 0x00},
                    {'R', 0x201, 0x00}},
            walked);
    arch = &plain;
    TEST_ASSERT_TRUE(walked);
    TEST_ASSERT_EQUAL_STRING(". . . d d F1 F1", marks.c_str());
    TEST_ASSERT_TRUE(walker.interrupt(0));
}

// What the table check takes, and what it doesn't.
void test_ill_formed() {
    for (const auto &o : OPCODES)
        TEST_ASSERT_NULL_MESSAGE(MatchWalker::illFormed(o.seq), o.seq);
    TEST_ASSERT_NULL(MatchWalker::illFormed("1{R+1W-2A.r0}N"));
    TEST_ASSERT_NULL(MatchWalker::illFormed("12~[RrWwJ]"));
    TEST_ASSERT_NULL(MatchWalker::illFormed("1N/12N"));  // a variant's length
    TEST_ASSERT_NULL(MatchWalker::illFormed("XXwV@1XwV", "", false));
    TEST_ASSERT_NOT_NULL(MatchWalker::illFormed("1QN"));       // no token
    TEST_ASSERT_NOT_NULL(MatchWalker::illFormed("1{R{W}}N"));  // nested
    TEST_ASSERT_NOT_NULL(MatchWalker::illFormed("1{RN"));      // unclosed
    TEST_ASSERT_NOT_NULL(MatchWalker::illFormed("1R}N"));
    TEST_ASSERT_NOT_NULL(MatchWalker::illFormed("1[RJN]"));    // two fetches
    TEST_ASSERT_NOT_NULL(MatchWalker::illFormed("1[R-]N"));    // not in [ ]
    TEST_ASSERT_NOT_NULL(MatchWalker::illFormed("1R+1N"));     // a step outside
    TEST_ASSERT_NOT_NULL(MatchWalker::illFormed("12N@1N"));    // lengths
    TEST_ASSERT_NOT_NULL(MatchWalker::illFormed("1{RRRRRRRRRRRRRRRRR}N"));
    TEST_ASSERT_NOT_NULL(MatchWalker::illFormed("1#N"));
    TEST_ASSERT_NULL(MatchWalker::illFormed("1#N", "#"));
}

int main() {
    MatchWalker::trace = getenv("WALK_TRACE") != nullptr;
    UNITY_BEGIN();
    RUN_TEST(test_straight_line);
    RUN_TEST(test_conditional);
    RUN_TEST(test_backtrack_across_instructions);
    RUN_TEST(test_repeat_group);
    RUN_TEST(test_popped_value);
    RUN_TEST(test_ring_ends_inside);
    RUN_TEST(test_stop);
    RUN_TEST(test_earliest_start);
    RUN_TEST(test_no_walk);
    RUN_TEST(test_effective_address);
    RUN_TEST(test_hand_over);
    RUN_TEST(test_dummy_next_read);
    RUN_TEST(test_no_next_fetch_named);
    RUN_TEST(test_interrupt);
    RUN_TEST(test_interrupt_cut_before_vector);
    RUN_TEST(test_last_is_next_fetch);
    RUN_TEST(test_wraps_at_mask);
    RUN_TEST(test_word_cycles);
    RUN_TEST(test_unordered);
    RUN_TEST(test_unordered_read_at_target);
    RUN_TEST(test_resume);
    RUN_TEST(test_ill_formed);
    RUN_TEST(test_budget);
    RUN_TEST(test_word_fetch);
    RUN_TEST(test_ran_to_stop);
    RUN_TEST(test_lead_unproven);
    RUN_TEST(test_halt);
    RUN_TEST(test_interrupt_before_target);
    RUN_TEST(test_refetch);
    RUN_TEST(test_cut_start);
    RUN_TEST(test_mmu);
    RUN_TEST(test_group_step_decides);
    RUN_TEST(test_queue_fetches_ahead);
    RUN_TEST(test_queue_capacity);
    RUN_TEST(test_queue_flushed);
    RUN_TEST(test_queue_interrupt);
    RUN_TEST(test_unconfirmed_backtracks);
    RUN_TEST(test_last_is_next_fetch_cut);
    RUN_TEST(test_max_start);
    return UNITY_END();
}

// Local Variables:
// mode: c++
// c-basic-offset: 4
// tab-width: 4
// End:
// vim: set ft=cpp et ts=4 sw=4:
