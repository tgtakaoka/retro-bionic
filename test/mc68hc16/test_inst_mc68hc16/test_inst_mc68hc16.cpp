// Host-side test of the MC68HC16's instruction length table against
// libasm's disassembler: `pio test -e native`. With GEN_TABLE set, the
// built .pio/build/native/program prints the table for inst_mc68hc16.cpp
// instead.

#include <stdio.h>
#include <stdlib.h>
#include <unity.h>

#include <dis_mc68hc16.h>
#include "mc68hc16/inst_mc68hc16.cpp"

using namespace debugger::mc68hc16;

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

constexpr uint8_t PREFIX[] = {0, 0x17, 0x27, 0x37};

bool isPrefix(uint8_t op) {
    return op == 0x17 || op == 0x27 || op == 0x37;
}

// libasm's length of |page|:|op| with the operands filled with |fill|; 0
// when it does not decode. Fills are even, so branch targets stay aligned.
uint_fast8_t libasmLength(uint_fast8_t page, uint8_t op, uint8_t fill) {
    uint8_t code[16];
    for (auto &b : code)
        b = fill;
    uint8_t n = 0;
    if (page)
        code[n++] = PREFIX[page];
    code[n++] = op;
    Bytes mem(code, sizeof(code));
    libasm::mc68hc16::DisMc68HC16 dis;
    libasm::Insn insn(0);
    char operands[80];
    dis.decode(mem, insn, operands, sizeof(operands));
    return insn.getError() == libasm::OK ? insn.length() : 0;
}

// The length the table must hold: libasm's, the same under every fill, or
// 2 for an opcode libasm rejects.
uint_fast8_t expected(uint_fast8_t page, uint8_t op) {
    static constexpr uint8_t FILLS[] = {0x00, 0x02, 0xF0};
    uint_fast8_t len = 0;
    for (const auto fill : FILLS) {
        const auto n = libasmLength(page, op, fill);
        if (n == 0)
            continue;
        if (len && n != len) {
            char msg[60];
            snprintf(msg, sizeof(msg), "%02X:%02X length %u and %u",
                    PREFIX[page], op, unsigned(len), unsigned(n));
            TEST_FAIL_MESSAGE(msg);
        }
        len = n;
    }
    return len ? len : 2;
}

void printTable() {
    printf("// clang-format off\n");
    printf("constexpr uint8_t LENGTHS[4][64] = {\n");
    for (uint_fast8_t page = 0; page < 4; ++page) {
        printf("    {\n");
        for (unsigned op = 0; op < 256; op += 32) {
            printf("       ");
            for (unsigned b = op; b < op + 32; b += 4) {
                uint8_t packed = 0;
                for (unsigned i = 0; i < 4; ++i) {
                    const uint8_t o = b + i;
                    const auto len =
                            (page == 0 && isPrefix(o)) ? 2 : expected(page, o);
                    packed |= ((len / 2 - 1) & 3) << (i * 2);
                }
                printf(" 0x%02X,", packed);
            }
            printf("  // %02X-%02X\n", op, op + 31);
        }
        printf("    },\n");
    }
    printf("};\n");
    printf("// clang-format on\n");
}

// Every opcode on every page: the table agrees with libasm.
void test_lengths() {
    for (uint_fast8_t page = 0; page < 4; ++page) {
        for (unsigned op = 0; op < 256; ++op) {
            if (page == 0 && isPrefix(op))
                continue;  // the page's own entries cover these
            const auto want = expected(page, op);
            const uint8_t first = page ? PREFIX[page] : op;
            const uint8_t second = page ? op : 0;
            char msg[40];
            snprintf(msg, sizeof(msg), "%02X:%02X", PREFIX[page], op);
            TEST_ASSERT_EQUAL_MESSAGE(
                    want, InstMc68hc16::length(first, second), msg);
        }
    }
}

}  // namespace

void setUp() {}
void tearDown() {}

int main() {
    if (getenv("GEN_TABLE")) {
        printTable();
        return 0;
    }
    UNITY_BEGIN();
    RUN_TEST(test_lengths);
    return UNITY_END();
}

// Local Variables:
// mode: c++
// c-basic-offset: 4
// tab-width: 4
// End:
// vim: set ft=cpp et ts=4 sw=4:
