#include "inst_h16.h"

namespace debugger {
namespace h16 {

namespace {
// The signed displacement in the last |size| bytes of |code|, big-endian.
int32_t disp(const uint8_t *code, uint_fast8_t len, uint_fast8_t size) {
    const auto p = code + len - size;
    switch (size) {
    case 1:
        return static_cast<int8_t>(p[0]);
    case 2:
        return static_cast<int16_t>((p[0] << 8) | p[1]);
    default:
        return static_cast<int32_t>((uint32_t(p[0]) << 24) |
                                    (uint32_t(p[1]) << 16) | (p[2] << 8) | p[3]);
    }
}
}  // namespace

// A branch's displacement is its last field, sized by the low two bits of
// its first byte (Bcc:G's prefix), and counts from the next instruction.
InstH16::Flow InstH16::flow(const uint8_t *code, uint_fast8_t len,
        uint32_t addr, uint32_t &target) {
    static constexpr uint_fast8_t SIZES[] = {1, 2, 4};
    const auto op = code[0];
    switch (op) {
    case 0x98:  // BRA
    case 0x99:
    case 0x9A:
    case 0x9B:  // JMP
    case 0xA8:  // BSR
    case 0xA9:
    case 0xAA:
    case 0xAB:  // JSR
    case 0xB8:  // RTD
    case 0xB9:
    case 0xBA:
    case 0xBB:  // RTS
    case 0xF1:  // RTE
    case 0xF2:  // TRAPA
    case 0xF4:  // RTR
        return FLOW_JUMP;
    case 0xF8:  // ANDC
    case 0xF9:  // ORC
    case 0xFA:  // XORC
    case 0xFB:  // LDC
        return FLOW_SYNC;
    case 0xA4:  // Bcc:G
    case 0xA5:
    case 0xA6:
        if (len < 2)
            return FLOW_NEXT;
        if ((code[1] & 0xF) == 0)  // BT
            return FLOW_JUMP;
        if ((code[1] & 0xF) == 1)  // BF
            return FLOW_NEXT;
        break;
    case 0xA0:  // BEQ
    case 0xA1:
    case 0xA2:
    case 0xB0:  // BNE
    case 0xB1:
    case 0xB2:
    case 0xB4:  // SCB
    case 0xB5:
    case 0xB6:
        break;
    default:
        return FLOW_NEXT;
    }
    const auto size = SIZES[op & 3];
    if (len < size + 1)
        return FLOW_NEXT;
    target = (addr + len + disp(code, len, size)) & 0xFFFFFF;
    return FLOW_BRANCH;
}

}  // namespace h16
}  // namespace debugger

// Local Variables:
// mode: c++
// c-basic-offset: 4
// tab-width: 4
// End:
// vim: set ft=cpp et ts=4 sw=4:
