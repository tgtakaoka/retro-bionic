// Host-side test of the H16's control flow decode against libasm's
// disassembler: `pio test -e native`.

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unity.h>

#include <dis_h16.h>
#include "h16/inst_h16.cpp"

using namespace debugger::h16;

namespace {

constexpr uint32_t ORG = 0x10000;

struct Bytes final : libasm::DisMemory {
    Bytes(const uint8_t *bytes, uint8_t len)
        : libasm::DisMemory(ORG), _bytes(bytes), _len(len) {}
    bool hasNext() const override { return address() - ORG < _len; }
    uint8_t nextByte() override { return _bytes[address() - ORG]; }

private:
    const uint8_t *const _bytes;
    const uint8_t _len;
};

bool named(const char *name, const char *const *names) {
    for (; *names; ++names) {
        if (strcmp(name, *names) == 0)
            return true;
    }
    return false;
}

constexpr const char *JUMPS[] = {"BRA", "BT", "JMP", "BSR", "JSR", "RTD",
        "RTS", "RTE", "RTR", "TRAPA", nullptr};
constexpr const char *BRANCHES[] = {"BHI", "BLS", "BCC", "BCS", "BNE", "BEQ",
        "BVC", "BVS", "BPL", "BMI", "BGE", "BLT", "BGT", "BLE", "SCB",
        nullptr};
constexpr const char *SYNCS[] = {"ANDC", "ORC", "XORC", "LDC", nullptr};

// |code| as libasm decodes it at ORG and as flow() does; false when libasm
// rejects it.
bool check(uint8_t *code, uint8_t len) {
    Bytes mem(code, len);
    libasm::h16::DisH16 dis;
    libasm::Insn insn(ORG);
    char operands[80];
    dis.decode(mem, insn, operands, sizeof(operands));
    if (insn.getError() != libasm::OK)
        return false;
    char name[16];
    strncpy(name, insn.name(), sizeof(name) - 1);
    name[sizeof(name) - 1] = 0;
    name[strcspn(name, ".:/")] = 0;
    for (auto p = name; *p; ++p)
        *p = toupper(*p);
    InstH16::Flow want = InstH16::FLOW_NEXT;
    uint32_t to = 0;
    if (named(name, JUMPS)) {
        want = InstH16::FLOW_JUMP;
    } else if (named(name, SYNCS)) {
        want = InstH16::FLOW_SYNC;
    } else if (named(name, BRANCHES)) {
        want = InstH16::FLOW_BRANCH;
        const auto dollar = strrchr(operands, '$');
        TEST_ASSERT_NOT_NULL_MESSAGE(dollar, operands);
        to = strtoul(dollar + 1, nullptr, 16) & 0xFFFFFF;  // A23-A0
    }
    uint32_t target = 0;
    const auto got = InstH16::flow(code, insn.length(), ORG, target);
    char msg[120];
    snprintf(msg, sizeof(msg), "%02X %02X: %s %s", code[0], code[1],
            insn.name(), operands);
    TEST_ASSERT_EQUAL_MESSAGE(want, got, msg);
    if (want == InstH16::FLOW_BRANCH)
        TEST_ASSERT_EQUAL_HEX32_MESSAGE(to, target, msg);
    return true;
}

constexpr uint8_t FILLS[] = {0x00, 0x02, 0x7F, 0xF0};

// Every first byte, and every Bcc:G condition, under each operand fill:
// even fills keep branch targets aligned, 0xF0 makes them go back.
void test_flow() {
    auto decoded = 0u;
    for (unsigned op = 0; op < 256; ++op) {
        for (const auto fill : FILLS) {
            for (unsigned cc = 0; cc < 16; ++cc) {
                const auto bccG = op >= 0xA4 && op <= 0xA6;
                if (cc && !bccG)
                    break;
                uint8_t code[16];
                memset(code, fill, sizeof(code));
                code[0] = op;
                if (bccG)
                    code[1] = cc;
                decoded += check(code, sizeof(code));
            }
        }
    }
    TEST_ASSERT_GREATER_THAN(500, decoded);
}

}  // namespace

void setUp() {}
void tearDown() {}

int main() {
    UNITY_BEGIN();
    RUN_TEST(test_flow);
    return UNITY_END();
}

// Local Variables:
// mode: c++
// c-basic-offset: 4
// tab-width: 4
// End:
// vim: set ft=cpp et ts=4 sw=4:
