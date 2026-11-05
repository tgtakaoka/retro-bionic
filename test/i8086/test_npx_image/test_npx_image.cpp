// Host-side tests for the FSAVE image decode: `pio test -e native`.

#include <unity.h>

#include <string.h>

#include "i8086/npx_i8086.h"

using namespace debugger::i8086;

namespace {

void put16(NpxImage &img, int off, uint16_t v) {
    img.bytes[off] = v;
    img.bytes[off + 1] = v >> 8;
}

void putReal(NpxImage &img, int i, uint16_t exp, uint64_t sig) {
    for (int b = 0; b < 8; ++b)
        img.bytes[14 + 10 * i + b] = sig >> (8 * b);
    put16(img, 14 + 10 * i + 8, exp);
}

// TOP=6: ST(0) is register 6, ST(1) register 7, ST(2) register 0.
NpxImage image() {
    NpxImage img;
    memset(img.bytes, 0, sizeof(img.bytes));
    put16(img, 0, 0x037F);
    put16(img, 2, 0x3000);  // TOP=6
    // Register 6 valid, 7 zero, the rest empty.
    put16(img, 4, 0x4FFF);
    put16(img, 6, 0x1092);
    put16(img, 8, 0x3106);  // IP 19-16 = 3, FLD DWORD (D9 06)
    put16(img, 10, 0x1039);
    put16(img, 12, 0x2000);  // OP 19-16 = 2
    putReal(img, 0, 0x3FFF, 0x8000000000000000ULL);  // 1.0
    putReal(img, 1, 0x8000, 0);                      // -0
    return img;
}

void test_words() {
    const auto img = image();
    TEST_ASSERT_EQUAL_HEX16(0x037F, img.cw());
    TEST_ASSERT_EQUAL_HEX16(0x3000, img.sw());
    TEST_ASSERT_EQUAL_HEX16(0x4FFF, img.tw());
    TEST_ASSERT_EQUAL_UINT8(6, img.top());
}

void test_pointers() {
    const auto img = image();
    TEST_ASSERT_EQUAL_HEX32(0x31092, img.ip());
    TEST_ASSERT_EQUAL_HEX16(0x106, img.opcode());
    TEST_ASSERT_EQUAL_HEX32(0x21039, img.op());
}

void test_tags_by_top() {
    const auto img = image();
    TEST_ASSERT_EQUAL_UINT8(0, img.tag(0));
    TEST_ASSERT_EQUAL_UINT8(1, img.tag(1));
    TEST_ASSERT_TRUE(img.empty(2));
    TEST_ASSERT_TRUE(img.empty(7));
    TEST_ASSERT_FALSE(img.empty(0));
    TEST_ASSERT_FALSE(img.empty(1));
}

void test_reals() {
    const auto img = image();
    TEST_ASSERT_EQUAL_HEX16(0x3FFF, img.exp(0));
    TEST_ASSERT_TRUE(img.sig(0) == 0x8000000000000000ULL);
    TEST_ASSERT_EQUAL_HEX16(0x8000, img.exp(1));
    TEST_ASSERT_TRUE(img.sig(1) == 0);
}

}  // namespace

void setUp() {}
void tearDown() {}

int main() {
    UNITY_BEGIN();
    RUN_TEST(test_words);
    RUN_TEST(test_pointers);
    RUN_TEST(test_tags_by_top);
    RUN_TEST(test_reals);
    return UNITY_END();
}
