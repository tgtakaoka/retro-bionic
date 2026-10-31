// Host-side test of the MC68000 family's register save: `pio test -e
// native`. libasm reads the injected sequences back, and a model of their
// pushes (M68000PRM MOVEM, MOVE, MOVEC) checks where SaveMc68000 finds each
// register.

#include <stdio.h>
#include <string.h>
#include <unity.h>

#include <dis_mc68000.h>
#include "mc68000/save_mc68000.h"

using namespace debugger::mc68000;

namespace {

struct Bytes final : libasm::DisMemory {
    Bytes(const uint8_t *bytes, uint8_t len)
        : libasm::DisMemory(0), _bytes(bytes), _len(len) {}
    bool hasNext() const override { return address() < _len; }
    uint8_t nextByte() override { return _bytes[address()]; }

private:
    const uint8_t *const _bytes;
    const uint8_t _len;
};

// Each instruction of |inst| as libasm reads it, from 0.
void checkListing(const char *cpu, const uint8_t *inst, uint8_t len,
        const char *const *want, uint8_t lines) {
    libasm::mc68000::DisMc68000 dis;
    libasm::Disassembler &base = dis;
    TEST_ASSERT_TRUE(base.setCpu(cpu));
    Bytes mem(inst, len);
    uint8_t i = 0;
    while (mem.hasNext()) {
        TEST_ASSERT_LESS_THAN_UINT8(lines, i);
        libasm::Insn insn(mem.address());
        char operands[40];
        dis.decode(mem, insn, operands, sizeof(operands));
        TEST_ASSERT_EQUAL_MESSAGE(libasm::OK, insn.getError(), want[i]);
        char got[60];
        snprintf(got, sizeof(got), "%s %s", insn.name(), operands);
        TEST_ASSERT_EQUAL_STRING(want[i], got);
        ++i;
    }
    TEST_ASSERT_EQUAL_UINT8(lines, i);
}

void test_save_listing() {
    static const char *const WANT[] = {
            "MOVEM.L D0-A6, -(A7)",
            "MOVE USP, A0",
            "MOVE.L A0, -(A7)",
            "MOVE.W SR, -(A7)",
            "BRA $000000",
    };
    checkListing(
            "MC68000", SaveMc68000::SAVE, sizeof(SaveMc68000::SAVE), WANT, 5);
}

void test_save10_listing() {
    static const char *const WANT[] = {
            "MOVEM.L D0-A6, -(A7)",
            "MOVE USP, A0",
            "MOVE.L A0, -(A7)",
            "MOVEC VBR, A0",
            "MOVE.L A0, -(A7)",
            "MOVEC SFC, A0",
            "MOVE.L A0, -(A7)",
            "MOVEC DFC, A0",
            "MOVE.L A0, -(A7)",
            "MOVE.W SR, -(A7)",
            "BRA $000000",
    };
    checkListing("MC68010", SaveMc68000::SAVE10, sizeof(SaveMc68000::SAVE10),
            WANT, 11);
}

// The writes the save makes: MOVEM -(An) stores A6 down to D0, so D0 is
// lowest; MOVE.L and MOVE.W -(A7) predecrement, high byte at the lower
// address. A long goes as two words, the high word first here; a word as
// one cycle on a 16-bit bus, two bytes on the MC68008.
struct Model {
    uint32_t mask;
    bool bus8;
    uint32_t sp;
    uint8_t buf[SaveMc68000::MAX_BYTES];
    uint32_t addrs[SaveMc68000::MAX_BYTES];
    uint8_t n = 0;

    Model(uint32_t ssp, uint32_t mask, bool bus8)
        : mask(mask), bus8(bus8), sp(ssp) {}

    void byte(uint32_t addr, uint8_t v) {
        buf[n] = v;
        addrs[n] = addr & mask;
        ++n;
    }
    void word(uint32_t addr, uint16_t v) {
        if (bus8) {
            // ATTENTION: unmeasured which byte the MC68008 writes first;
            // the decode must not care.
            byte(addr + 1, v);
            byte(addr, v >> 8);
        } else {
            byte(addr, v >> 8);
            byte(addr + 1, v);
        }
    }
    void pushLong(uint32_t v) {
        sp -= 4;
        word(sp, v >> 16);
        word(sp + 2, v);
    }
    void pushWord(uint16_t v) {
        sp -= 2;
        word(sp, v);
    }
    // MOVEM.L D0-D7/A0-A6,-(A7): A6 first, at the highest address.
    void movem(const uint32_t *d, const uint32_t *a) {
        for (int i = 6; i >= 0; --i)
            pushLong(a[i]);
        for (int i = 7; i >= 0; --i)
            pushLong(d[i]);
    }
};

const uint32_t D[8] = {0x00112233, 0x11223344, 0x22334455, 0x33445566,
        0x44556677, 0x55667788, 0x66778899, 0x778899AA};
const uint32_t A[7] = {0x8899AABB, 0x99AABBCC, 0xAABBCCDD, 0xBBCCDDEE,
        0xCCDDEEFF, 0xDDEEFF00, 0xEEFF0011};
constexpr uint32_t USP = 0x00012340;
constexpr uint32_t VBR = 0x00ABC000;
constexpr uint8_t SFC = 5, DFC = 1;
constexpr uint16_t SR = 0x2704;

void checkSave(uint32_t ssp, uint32_t mask, bool bus8, bool mc68010) {
    Model m(ssp, mask, bus8);
    m.movem(D, A);
    m.pushLong(USP);
    if (mc68010) {
        m.pushLong(VBR);
        m.pushLong(SFC);
        m.pushLong(DFC);
    }
    m.pushWord(SR);
    TEST_ASSERT_EQUAL_UINT8(SaveMc68000::bytes(mc68010), m.n);

    SaveMc68000::Pushed p;
    TEST_ASSERT_TRUE(
            SaveMc68000::decode(m.buf, m.addrs, m.n, mask, mc68010, p));
    TEST_ASSERT_EQUAL_HEX32(ssp & mask, p.sp);
    TEST_ASSERT_EQUAL_HEX16(SR, p.sr);
    TEST_ASSERT_EQUAL_HEX32(USP, p.usp);
    TEST_ASSERT_EQUAL_HEX32_ARRAY(D, p.d, 8);
    TEST_ASSERT_EQUAL_HEX32_ARRAY(A, p.a, 7);
    if (mc68010) {
        TEST_ASSERT_EQUAL_HEX32(VBR, p.vbr);
        TEST_ASSERT_EQUAL_UINT8(SFC, p.sfc);
        TEST_ASSERT_EQUAL_UINT8(DFC, p.dfc);
    }
}

constexpr uint32_t MASK_24 = 0xFFFFFF;   // MC68000, MC68010
constexpr uint32_t MASK_20 = 0x0FFFFF;   // MC68008P
constexpr uint32_t ORG_PARK = 0x0FF000;  // where reset parks the CPU

void test_save_mc68000() {
    checkSave(ORG_PARK, MASK_24, false, false);
}

void test_save_mc68010() {
    checkSave(ORG_PARK, MASK_24, false, true);
}

void test_save_mc68008() {
    checkSave(ORG_PARK, MASK_20, true, false);
}

void test_save_hc001_8bit() {
    checkSave(ORG_PARK, MASK_24, true, false);
}

// A stack just above 0 pushes across the top of the address space.
void test_save_wraps() {
    checkSave(0x000020, MASK_24, false, false);
    checkSave(0x000020, MASK_24, false, true);
    checkSave(0x000010, MASK_20, true, false);
}

// A byte the bus never moved fails the decode.
void test_save_missing_byte() {
    Model m(ORG_PARK, MASK_24, false);
    m.movem(D, A);
    m.pushLong(USP);
    m.pushWord(SR);
    m.addrs[0] = 0;  // D0's high byte lost
    SaveMc68000::Pushed p;
    TEST_ASSERT_FALSE(
            SaveMc68000::decode(m.buf, m.addrs, m.n, MASK_24, false, p));
}

}  // namespace

void setUp() {}
void tearDown() {}

int main() {
    UNITY_BEGIN();
    RUN_TEST(test_save_listing);
    RUN_TEST(test_save10_listing);
    RUN_TEST(test_save_mc68000);
    RUN_TEST(test_save_mc68010);
    RUN_TEST(test_save_mc68008);
    RUN_TEST(test_save_hc001_8bit);
    RUN_TEST(test_save_wraps);
    RUN_TEST(test_save_missing_byte);
    return UNITY_END();
}

// Local Variables:
// mode: c++
// c-basic-offset: 4
// tab-width: 4
// End:
// vim: set ft=cpp et ts=4 sw=4:
