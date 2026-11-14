// Host-side tests for the queue status decoder: `pio test -e native`.

#include <unity.h>

#include <map>
#include <string>
#include <vector>

#include "i8086/qs_decode_i8086.h"

using namespace debugger::i8086;

namespace {

// A word bus, each fetch bringing the even word, or a byte bus; a time is
// a CLK count, and
// QS is 0 (none), 1 (first byte), 2 (empty), 3 (subsequent byte).
struct Bus {
    struct Cycle {
        uint32_t time;
        bool fetch;
        uint32_t addr;
        bool vector;
        int md;  // a data read's MD, or -1
    };
    std::vector<Cycle> cyc;
    std::vector<uint8_t> qsAt;  // indexed by time
    std::map<uint32_t, uint32_t> len;
    std::map<uint32_t, uint8_t> mem;
    std::string log;
    bool byteBus = false;

    void fetch(uint32_t time, uint32_t addr) {
        cyc.push_back({time, true, addr, false, -1});
    }
    void data(uint32_t time, uint32_t addr, int md = -1) {
        cyc.push_back({time, false, addr, false, md});
    }
    void vec(uint32_t time, uint32_t addr) {
        cyc.push_back({time, false, addr, true, -1});
    }
    void qs(uint32_t time, uint8_t q) {
        if (qsAt.size() <= time)
            qsAt.resize(time + 1, 0);
        qsAt[time] = q;
    }

    int cycles() const { return cyc.size(); }
    uint32_t cycleTime(int i) const { return cyc[i].time; }
    bool fetch(int i, uint32_t &first, uint32_t &next) const {
        if (!cyc[i].fetch)
            return false;
        first = cyc[i].addr;
        next = byteBus ? cyc[i].addr + 1 : (cyc[i].addr | 1) + 1;
        return true;
    }
    int samples() const { return qsAt.size(); }
    uint32_t sampleTime(int k) const { return k; }
    uint8_t qs(int k) const { return qsAt[k]; }
    uint8_t length(uint32_t addr, bool) const {
        const auto l = len.find(addr);
        return l == len.end() ? 1 : l->second;
    }
    uint32_t insn(uint32_t addr, bool emu) {
        log += (emu ? "E" : "I") + std::to_string(addr) + " ";
        return addr + length(addr, emu);
    }
    void cycle(int i) { log += "D" + std::to_string(cyc[i].addr) + " "; }
    bool vector(int i) const { return cyc[i].vector; }
    int popMode(int i) const { return cyc[i].fetch ? -2 : cyc[i].md; }
    uint8_t code(uint32_t addr) const {
        const auto m = mem.find(addr);
        return m == mem.end() ? 0x90 : m->second;
    }
    uint32_t areaBegin = 1, areaEnd = 0;  // none
    int areaMode(uint32_t addr) const {
        if (areaBegin > areaEnd)
            return -1;
        return addr >= areaBegin && addr <= areaEnd;
    }
};

using Decoder = QsDecoder<Bus, 16>;

// Stream 0 at 50 is the one begun before the samples; 100 starts anew.
void prologue(Bus &bus) {
    bus.fetch(0, 50);
    bus.fetch(2, 100);
}

void test_straight() {
    Bus bus;
    bus.len = {{100, 3}, {103, 1}, {104, 2}};
    prologue(bus);
    bus.fetch(6, 102);
    bus.qs(5, 1);  // 100
    bus.qs(6, 3);
    bus.qs(7, 3);
    bus.qs(8, 1);  // 103
    bus.data(9, 2000);
    bus.fetch(10, 104);
    bus.qs(12, 1);  // 104
    bus.qs(13, 3);
    Decoder dec(bus, true);
    TEST_ASSERT_TRUE(dec.decode());
    TEST_ASSERT_EQUAL_STRING("I100 I103 D2000 I104 ", bus.log.c_str());
    TEST_ASSERT_EQUAL(6, dec.good);
    TEST_ASSERT_EQUAL(0, dec.bad);
}

void test_jump() {
    Bus bus;
    bus.len = {{100, 2}, {200, 1}};
    prologue(bus);
    bus.qs(5, 1);  // 100 JMP
    bus.qs(6, 3);
    bus.qs(7, 2);
    bus.fetch(8, 200);
    bus.qs(11, 1);  // 200
    Decoder dec(bus, true);
    TEST_ASSERT_TRUE(dec.decode());
    TEST_ASSERT_EQUAL_STRING("I100 I200 ", bus.log.c_str());
    TEST_ASSERT_EQUAL(0, dec.bad);
}

// A branch taken to the very address the next prefetch would have asked
// for: only the empty queue (QS 10) tells the new stream.
void test_taken_to_sequential() {
    const auto run = [](bool pairs, std::string &log) {
        Bus bus;
        bus.len = {{100, 2}, {102, 1}, {103, 2}, {104, 2}};
        prologue(bus);
        bus.fetch(6, 102);
        bus.qs(5, 1);  // 100 JB 104
        bus.qs(6, 3);
        bus.qs(9, 2);
        bus.fetch(10, 104);
        bus.qs(13, 1);  // 104
        bus.qs(14, 3);
        Decoder dec(bus, pairs);
        TEST_ASSERT_TRUE(dec.decode());
        log = bus.log;
        return dec.bad;
    };
    std::string log;
    TEST_ASSERT_EQUAL(0, run(true, log));
    TEST_ASSERT_EQUAL_STRING("I100 I104 ", log.c_str());
    // QS0 alone runs on into the bytes the branch skipped.
    run(false, log);
    TEST_ASSERT_EQUAL_STRING("I100 I102 I103 ", log.c_str());
}

// The new stream's first byte taken no later than its fetch's timestamp,
// as a V30, prompt at taking bytes, shows when the timestamp runs late:
// the flush, not the time, hands the byte to the new stream.
void test_take_at_fetch_time() {
    const auto run = [](bool pairs, std::string &log) {
        Bus bus;
        bus.len = {{100, 2}, {200, 1}, {201, 1}};
        prologue(bus);
        bus.qs(5, 1);  // 100 JMP 200
        bus.qs(6, 3);
        bus.qs(7, 2);
        bus.fetch(9, 200);
        bus.qs(9, 1);  // 200
        bus.qs(10, 1);  // 201
        Decoder dec(bus, pairs);
        TEST_ASSERT_TRUE(dec.decode());
        log = bus.log;
        return dec.bad;
    };
    std::string log;
    TEST_ASSERT_EQUAL(0, run(true, log));
    TEST_ASSERT_EQUAL_STRING("I100 I200 I201 ", log.c_str());
    // By time alone the byte at 9 stays with the old stream, and the new
    // one starts a byte late.
    run(false, log);
    TEST_ASSERT_EQUAL_STRING("I100 I102 I200 ", log.c_str());
}

// A QS 11 where the decode starts an instruction, or a 01 where it
// doesn't, counts as bad: QS1 sampled off its own clock.
void test_bad_starts() {
    Bus bus;
    bus.len = {{100, 2}, {102, 2}};
    prologue(bus);
    bus.fetch(6, 102);
    bus.qs(5, 3);  // should be 01
    bus.qs(6, 1);  // should be 11
    bus.qs(7, 1);
    bus.qs(8, 3);
    Decoder dec(bus, true);
    TEST_ASSERT_TRUE(dec.decode());
    TEST_ASSERT_EQUAL_STRING("I100 I102 ", bus.log.c_str());
    TEST_ASSERT_EQUAL(2, dec.good);
    TEST_ASSERT_EQUAL(2, dec.bad);
}

// The instruction at CS:IP, its first byte taken as the CPU stopped, is
// left out with the stop's own after it; one at the same address earlier,
// as in a loop, stays.
void test_stop_at() {
    Bus bus;
    bus.len = {{100, 1}, {101, 2}, {103, 1}};
    prologue(bus);
    bus.fetch(6, 102);
    bus.qs(5, 1);  // 100
    bus.qs(6, 1);  // 101 JMP 100
    bus.qs(7, 3);
    bus.qs(8, 2);
    bus.fetch(9, 100);
    bus.qs(12, 1);  // 100
    bus.qs(13, 1);  // 101, where the CPU stopped
    bus.qs(15, 2);  // the stop
    bus.qs(17, 1);  // the stop's own
    bus.qs(18, 3);
    Decoder dec(bus, true, 101);
    TEST_ASSERT_TRUE(dec.decode());
    TEST_ASSERT_EQUAL_STRING("I100 I101 I100 ", bus.log.c_str());
}

// Stopped at the top of a loop, before taking its first byte again: the
// earlier pass at that address stays, and so does all after it.
void test_stop_at_loop_top() {
    Bus bus;
    bus.len = {{100, 1}, {101, 2}};
    prologue(bus);
    bus.qs(5, 1);  // 100
    bus.qs(6, 1);  // 101 JMP 100
    bus.qs(7, 3);
    bus.qs(8, 2);
    bus.fetch(9, 100);
    bus.qs(14, 2);  // the stop, 100 not taken
    bus.qs(16, 1);  // the stop's own
    Decoder dec(bus, true, 100);
    TEST_ASSERT_TRUE(dec.decode());
    TEST_ASSERT_EQUAL_STRING("I100 I101 ", bus.log.c_str());
    TEST_ASSERT_EQUAL(0, dec.bad);
}

// A byte-wide run of code begun before the samples, alone in them: the
// stop places it. Bytes 100..109 hold instructions of 2, 1, 3, 2, 2.
void lone_stream(Bus &bus) {
    bus.len = {{100, 2}, {102, 1}, {103, 3}, {106, 2}, {108, 2}, {110, 1}};
    bus.byteBus = true;
    for (uint32_t a = 101; a <= 111; ++a)
        bus.fetch((a - 101) * 4, a);
    // Takes from 99 on: 99 ends the instruction before 100.
    const uint8_t qs[] = {3, 1, 3, 1, 1, 3, 3, 1, 3, 1, 3};
    for (uint32_t i = 0; i < sizeof(qs); ++i)
        bus.qs(2 + i * 4, qs[i]);
}

// Stopped before taking 110: its last byte taken is 109.
void test_lone_stream_before_stop() {
    Bus bus;
    lone_stream(bus);
    bus.qs(50, 2);  // the stop
    bus.qs(54, 1);  // the stop's own
    Decoder dec(bus, true, 110);
    TEST_ASSERT_TRUE(dec.decode());
    TEST_ASSERT_EQUAL_STRING("I100 I102 I103 I106 I108 ", bus.log.c_str());
    TEST_ASSERT_EQUAL(0, dec.bad);
}

// Stopped after taking 110's first byte: it's left out.
void test_lone_stream_at_stop() {
    Bus bus;
    lone_stream(bus);
    bus.qs(46, 1);  // 110
    bus.qs(50, 2);  // the stop
    Decoder dec(bus, true, 110);
    TEST_ASSERT_TRUE(dec.decode());
    TEST_ASSERT_EQUAL_STRING("I100 I102 I103 I106 I108 ", bus.log.c_str());
}

// Without QS1, or with lengths the QS 01s don't fit, it stays unknown.
void test_lone_stream_unplaced() {
    Bus bus;
    lone_stream(bus);
    bus.qs(50, 2);
    Decoder qs0(bus, false, 110);
    TEST_ASSERT_FALSE(qs0.decode());
    Decoder wrong(bus, true, 111);
    TEST_ASSERT_FALSE(wrong.decode());
}

// A backtrace limit shows the last instructions, and the cycles from the
// first of them on.
void test_limit() {
    Bus bus;
    bus.len = {{100, 3}, {103, 1}, {104, 2}};
    prologue(bus);
    bus.fetch(6, 102);
    bus.qs(5, 1);  // 100
    bus.qs(6, 3);
    bus.qs(7, 3);
    bus.data(7, 2000);
    bus.qs(8, 1);  // 103
    bus.data(9, 2001);
    bus.fetch(10, 104);
    bus.qs(12, 1);  // 104
    bus.qs(13, 3);
    Decoder dec(bus, true);
    TEST_ASSERT_TRUE(dec.decode(2));
    TEST_ASSERT_EQUAL_STRING("I103 D2001 I104 ", bus.log.c_str());
}

// Nothing but the instruction the CPU stopped at: no backtrace to show.
void test_limit_nothing() {
    Bus bus;
    bus.len = {{100, 1}};
    prologue(bus);
    bus.qs(5, 1);  // 100, where the CPU stopped
    bus.qs(7, 2);  // the stop
    Decoder dec(bus, true, 100);
    TEST_ASSERT_FALSE(dec.decode(5));
    TEST_ASSERT_EQUAL_STRING("", bus.log.c_str());
}

// BRKEM enters 8080 code, an interrupt leaves it for a native handler,
// and IRET's flags pop (MD 0) returns to it; 8080 code shows as E.
void test_emulation() {
    Bus bus;
    bus.len = {{100, 3}, {200, 1}, {201, 1}, {300, 1}};
    bus.mem = {{100, 0x0F}, {101, 0xFF}, {300, 0xCF}};
    prologue(bus);
    bus.qs(5, 1);  // 100 BRKEM
    bus.qs(6, 3);
    bus.qs(7, 3);
    bus.qs(8, 2);
    bus.vec(9, 0x88);
    bus.fetch(10, 200);
    bus.qs(13, 1);  // 200, 8080
    bus.qs(14, 2);  // an interrupt
    bus.vec(15, 0x3BC);
    bus.fetch(16, 300);
    bus.qs(19, 1);  // 300 IRET, native
    bus.data(20, 0xFF0);
    bus.data(21, 0xFF2);
    bus.data(22, 0xFF4, 0);  // flags, MD 0
    bus.qs(23, 2);
    bus.fetch(24, 201);
    bus.qs(27, 1);  // 201, 8080 again
    Decoder dec(bus, true, UINT32_MAX, true);
    TEST_ASSERT_TRUE(dec.decode());
    TEST_ASSERT_EQUAL_STRING(
            "I100 D136 E200 D956 I300 D4080 D4082 D4084 E201 ",
            bus.log.c_str());
}

// In 8080 code CF is RST 1, which pushes: the mode stays. RETEM (ED FD)
// returns to native code.
void test_emulation_lookalikes() {
    Bus bus;
    bus.len = {{100, 1}, {8, 2}, {500, 1}};
    bus.mem = {{100, 0xCF}, {8, 0xED}, {9, 0xFD}};
    prologue(bus);
    bus.qs(5, 1);  // 100 RST 1
    bus.qs(6, 2);
    bus.data(7, 0xFFE, 0);  // a push: no MD
    bus.data(8, 0xFFF);
    bus.fetch(9, 8);
    bus.qs(12, 1);  // 8 RETEM
    bus.qs(13, 3);
    bus.data(14, 0xFF0, 1);
    bus.qs(15, 2);
    bus.fetch(16, 500);
    bus.qs(19, 1);  // 500, native
    Decoder dec(bus, true, UINT32_MAX, false);
    TEST_ASSERT_TRUE(dec.decode());
    // The stream at 100 was 8080 code, as the RETEM after it shows.
    TEST_ASSERT_EQUAL_STRING(
            "E100 D4094 D4095 E8 D4080 I500 ", bus.log.c_str());
}

// An 8080EMU area decides a stream's mode by its address, over what began
// it: here a RETEM look-alike in native code changes nothing.
void test_emulation_area() {
    Bus bus;
    bus.len = {{100, 2}, {500, 1}};
    bus.mem = {{100, 0xED}, {101, 0xFD}};
    bus.areaBegin = 0x10000;
    bus.areaEnd = 0x1FFFF;
    prologue(bus);
    bus.qs(5, 1);  // 100 IN AX,DX; STD as native code
    bus.qs(6, 3);
    bus.qs(7, 2);
    bus.fetch(9, 500);
    bus.qs(12, 1);  // 500
    Decoder dec(bus, true, UINT32_MAX, false);
    TEST_ASSERT_TRUE(dec.decode());
    TEST_ASSERT_EQUAL_STRING("I100 I500 ", bus.log.c_str());
}

// Only the stream begun before the samples: nothing to show.
void test_first_stream_only() {
    Bus bus;
    bus.fetch(0, 50);
    bus.fetch(4, 52);
    bus.qs(3, 1);
    Decoder dec(bus, true);
    TEST_ASSERT_FALSE(dec.decode());
}

// Cycles held from the run's start: the first stream begins at its first
// fetch, alone or before a jump.
void test_from_start() {
    Bus bus;
    bus.len = {{100, 3}, {103, 1}, {104, 1}};
    bus.fetch(0, 100);
    bus.fetch(4, 102);
    bus.qs(3, 1);  // 100
    bus.qs(4, 3);
    bus.qs(5, 3);
    bus.qs(7, 1);  // 103
    Decoder lone(bus, true, 104, false, true);
    TEST_ASSERT_TRUE(lone.decode());
    TEST_ASSERT_EQUAL_STRING("I100 I103 ", bus.log.c_str());
    TEST_ASSERT_EQUAL(0, lone.bad);
}

void test_from_start_then_jump() {
    Bus bus;
    bus.len = {{100, 2}, {200, 1}, {201, 1}};
    bus.fetch(0, 100);
    bus.qs(3, 1);  // 100: a jump
    bus.qs(4, 3);
    bus.qs(6, 2);
    bus.fetch(8, 200);
    bus.qs(10, 1);  // 200
    Decoder from(bus, true, 201, false, true);
    TEST_ASSERT_TRUE(from.decode());
    TEST_ASSERT_EQUAL_STRING("I100 I200 ", bus.log.c_str());
    bus.log.clear();
    Decoder before(bus, true, 201);
    TEST_ASSERT_TRUE(before.decode());
    TEST_ASSERT_EQUAL_STRING("I200 ", bus.log.c_str());
}

}  // namespace

void setUp() {}
void tearDown() {}

int main() {
    UNITY_BEGIN();
    RUN_TEST(test_straight);
    RUN_TEST(test_jump);
    RUN_TEST(test_taken_to_sequential);
    RUN_TEST(test_take_at_fetch_time);
    RUN_TEST(test_bad_starts);
    RUN_TEST(test_stop_at);
    RUN_TEST(test_stop_at_loop_top);
    RUN_TEST(test_lone_stream_before_stop);
    RUN_TEST(test_lone_stream_at_stop);
    RUN_TEST(test_lone_stream_unplaced);
    RUN_TEST(test_limit);
    RUN_TEST(test_limit_nothing);
    RUN_TEST(test_emulation);
    RUN_TEST(test_emulation_lookalikes);
    RUN_TEST(test_emulation_area);
    RUN_TEST(test_first_stream_only);
    RUN_TEST(test_from_start);
    RUN_TEST(test_from_start_then_jump);
    return UNITY_END();
}
