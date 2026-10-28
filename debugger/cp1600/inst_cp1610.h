#ifndef __INST_CP1610_H__
#define __INST_CP1610_H__

#include <stdint.h>

namespace debugger {
namespace cp1600 {

struct InstCp1610 {
    // Address the CPU starts at; Intellivision starts there.
    static constexpr uint16_t RESET_PC = 0x1000;

    static constexpr uint16_t HLT = 00000;
    static constexpr uint16_t SDBD = 00001;
    static constexpr uint16_t NOP = 00064;
    static constexpr uint16_t SIN = 00066;
    static constexpr uint16_t GSWD = 00060;  // GSWD Rd (R0~R3)
    static constexpr uint16_t RSWD = 00070;  // RSWD Rs
    static constexpr uint16_t MVO = 01100;   // MVO Rs, addr
    static constexpr uint16_t MVII = 01270;  // MVII data, Rd
    static constexpr uint16_t J = 00004;     // J/JE/JD addr

    static constexpr bool sin(uint16_t inst) { return (inst & 0x3FE) == SIN; }
    static constexpr uint16_t mvo(uint_fast8_t rs) { return MVO | rs; }
    static constexpr uint16_t mvii(uint_fast8_t rd) { return MVII | rd; }
    static constexpr uint16_t gswd(uint_fast8_t rd) { return GSWD | rd; }
    static constexpr uint16_t rswd(uint_fast8_t rs) { return RSWD | rs; }
    // Second and third words of J (ie=0), JE (ie=1) or JD (ie=2).
    static constexpr uint16_t j2(uint16_t addr, uint_fast8_t ie) {
        return 0x300 | ((addr >> 8) & 0xFC) | ie;
    }
    static constexpr uint16_t j3(uint16_t addr) { return addr & 0x3FF; }

    /**
     * Bus transactions following the fetch of |inst| before the next
     * fetch: operand words, ADAR and data reads/writes. |sdbd| tells
     * that the previous instruction was SDBD.
     */
    static uint_fast8_t busCycles(uint16_t inst, bool sdbd);
};

}  // namespace cp1600
}  // namespace debugger
#endif

// Local Variables:
// mode: c++
// c-basic-offset: 4
// tab-width: 4
// End:
// vim: set ft=cpp et ts=4 sw=4:
