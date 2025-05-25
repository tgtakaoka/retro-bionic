#include "inst_tms3201x.h"

#include "debugger.h"

namespace debugger {
namespace tms3201x {

constexpr uint8_t E(uint8_t p0, uint8_t p1, uint8_t p2, uint8_t p3) {
    return static_cast<uint8_t>((p0 << 0) | (p1 << 2) | (p2 << 4) | (p3 << 6));
}

constexpr uint_fast8_t C(const uint8_t *table, uint_fast8_t index) {
    const auto bit = (index & 3) << 1;
    const auto pos = index >> 2;
    return (table[pos] >> bit) & 3;
}

constexpr uint8_t PAGE7F[8] = {
        E(1, 1, 1, 0),  // 80~83
        E(0, 0, 0, 0),  // 84~87
        E(1, 1, 1, 1),  // 88~8B
        E(2, 2, 1, 1),  // 8C~8F
        E(1, 0, 0, 0),  // 90~93
        E(0, 0, 0, 0),  // 94~97
        E(0, 0, 0, 0),  // 98~9B
        E(2, 2, 0, 0),  // 9C~9F
};

constexpr uint8_t CYCLES[0x40] = {
        E(1, 1, 1, 1),  // 00~03
        E(1, 1, 1, 1),  // 04~07
        E(1, 1, 1, 1),  // 08~0B
        E(1, 1, 1, 1),  // 0C~0F
        E(1, 1, 1, 1),  // 10~13
        E(1, 1, 1, 1),  // 14~17
        E(1, 1, 1, 1),  // 18~1B
        E(1, 1, 1, 1),  // 1C~1F
        E(1, 1, 1, 1),  // 20~23
        E(1, 1, 1, 1),  // 24~27
        E(1, 1, 1, 1),  // 28~2B
        E(1, 1, 1, 1),  // 2C~2F
        E(1, 1, 0, 0),  // 30~33
        E(0, 0, 0, 0),  // 34~37
        E(1, 1, 0, 0),  // 38~3B
        E(0, 0, 0, 0),  // 3C~3F
        E(2, 2, 2, 2),  // 40~43
        E(2, 2, 2, 2),  // 44~47
        E(2, 2, 2, 2),  // 48~4B
        E(2, 2, 2, 2),  // 4C~4F
        E(1, 0, 0, 0),  // 50~53
        E(0, 0, 0, 0),  // 54~57
        E(1, 1, 1, 1),  // 58~5B
        E(1, 1, 1, 1),  // 5C~5F
        E(1, 1, 1, 1),  // 60~63
        E(1, 1, 1, 3),  // 64~67
        E(1, 1, 1, 1),  // 68~6B
        E(1, 1, 1, 1),  // 6C~6F
        E(1, 1, 0, 0),  // 70~73
        E(0, 0, 0, 0),  // 74~77
        E(1, 1, 1, 1),  // 78~7B
        E(1, 3, 1, 0),  // 7C~7F
        E(1, 1, 1, 1),  // 80~83
        E(1, 1, 1, 1),  // 84~87
        E(1, 1, 1, 1),  // 88~8B
        E(1, 1, 1, 1),  // 8C~8F
        E(1, 1, 1, 1),  // 90~93
        E(1, 1, 1, 1),  // 94~97
        E(1, 1, 1, 1),  // 98~9B
        E(1, 1, 1, 1),  // 9C~9F
        E(0, 0, 0, 0),  // A0~A3
        E(0, 0, 0, 0),  // A4~A7
        E(0, 0, 0, 0),  // A8~AB
        E(0, 0, 0, 0),  // AC~AF
        E(0, 0, 0, 0),  // B0~B3
        E(0, 0, 0, 0),  // B4~B7
        E(0, 0, 0, 0),  // B8~BB
        E(0, 0, 0, 0),  // BC~BF
        E(0, 0, 0, 0),  // C0~C3
        E(0, 0, 0, 0),  // C4~C7
        E(0, 0, 0, 0),  // C8~CB
        E(0, 0, 0, 0),  // CC~CF
        E(0, 0, 0, 0),  // D0~D3
        E(0, 0, 0, 0),  // D4~D7
        E(0, 0, 0, 0),  // D8~DB
        E(0, 0, 0, 0),  // DC~DF
        E(0, 0, 0, 0),  // E0~E3
        E(0, 0, 0, 0),  // E4~E7
        E(0, 0, 0, 0),  // E8~EB
        E(0, 0, 0, 0),  // EC~EF
        E(0, 0, 0, 0),  // F0~F3
        E(2, 2, 2, 0),  // F4~F7
        E(2, 2, 2, 2),  // F8~FB
        E(2, 2, 2, 2),  // FC~FF
};

uint_fast8_t InstTms3201X::cycles(uint16_t inst) {
    if ((inst & 0xFFE0) == 0x7F80)
        return C(PAGE7F, inst & 0x1F);
    return C(CYCLES, inst >> 8);
}

}  // namespace tms3201x
}  // namespace debugger

// Local Variables:
// mode: c++
// c-basic-offset: 4
// tab-width: 4
// End:
// vim: set ft=cpp et ts=4 sw=4:
