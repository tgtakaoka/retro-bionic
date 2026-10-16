#ifndef __INST_TMS320C2X_H__
#define __INST_TMS320C2X_H__

#include <stdint.h>

namespace debugger {
namespace tms320c2x {

struct InstTms320C2X {
    /** reset and interrupt vectors */
    static constexpr uint16_t ORG_RESET = 0x0000;
    static constexpr uint16_t ORG_INT0 = 0x0002;
    static constexpr uint16_t ORG_INT1 = 0x0004;
    static constexpr uint16_t ORG_INT2 = 0x0006;
    static constexpr uint16_t ORG_TINT = 0x0018;
    static constexpr uint16_t ORG_RINT = 0x001A;
    static constexpr uint16_t ORG_XINT = 0x001C;
    static bool isVector(uint16_t addr) {
        return (addr >= ORG_INT0 && addr <= ORG_INT2 && (addr & 1) == 0) ||
               addr == ORG_TINT || addr == ORG_RINT || addr == ORG_XINT;
    }

    static constexpr uint16_t NOP = 0x5500;
    static constexpr uint16_t DINT = 0xCE01;
    static constexpr uint16_t EINT = 0xCE00;
    static constexpr uint16_t PAC = 0xCE14;
    static constexpr uint16_t PUSH = 0xCE1C;
    static constexpr uint16_t POP = 0xCE1D;
    /** branch, followed by the target */
    static constexpr uint16_t B = 0xFF80;
    /** ST0's interrupt mode bit, set when disabled */
    static constexpr uint16_t ST0_INTM = 0x0200;

    /** indirect addressing through the current AR, nothing changed */
    static constexpr uint8_t IND = 0x80;

    /** load ARn with the next word */
    static constexpr uint16_t LRLK(uint_fast8_t ar) {
        return 0xD000 | (ar << 8);
    }
    /** load ACC with the next word */
    static constexpr uint16_t LALK = 0xD001;
    /** load ARP */
    static constexpr uint16_t LARP(uint_fast8_t ar) { return 0x5588 | ar; }
    /** load DP */
    static constexpr uint16_t LDPK(uint_fast16_t dp) {
        return 0xC800 | (dp & 0x1FF);
    }
    /** set the product shift mode */
    static constexpr uint16_t SPM(uint_fast8_t pm) { return 0xCE08 | (pm & 3); }
    /** multiply T by K into P */
    static constexpr uint16_t MPYK(uint_fast16_t k) {
        return 0xA000 | (k & 0x1FFF);
    }

    // |ma| is a direct address (0-7Fh) or IND
    static constexpr uint16_t IN(uint8_t ma, uint_fast8_t pa) {
        return 0x8000 | (pa << 8) | ma;
    }
    static constexpr uint16_t OUT(uint8_t ma, uint_fast8_t pa) {
        return 0xE000 | (pa << 8) | ma;
    }
    static constexpr uint16_t SACL(uint8_t ma) { return 0x6000 | ma; }
    static constexpr uint16_t SACH(uint8_t ma) { return 0x6800 | ma; }
    static constexpr uint16_t SAR(uint_fast8_t ar, uint8_t ma) {
        return 0x7000 | (ar << 8) | ma;
    }
    static constexpr uint16_t LAR(uint_fast8_t ar, uint8_t ma) {
        return 0x3000 | (ar << 8) | ma;
    }
    static constexpr uint16_t SST(uint8_t ma) { return 0x7800 | ma; }
    static constexpr uint16_t SST1(uint8_t ma) { return 0x7900 | ma; }
    static constexpr uint16_t LST(uint8_t ma) { return 0x5000 | ma; }
    static constexpr uint16_t LST1(uint8_t ma) { return 0x5100 | ma; }
    static constexpr uint16_t LT(uint8_t ma) { return 0x3C00 | ma; }
    static constexpr uint16_t MPY(uint8_t ma) { return 0x3800 | ma; }
    static constexpr uint16_t LPH(uint8_t ma) { return 0x5300 | ma; }
    static constexpr uint16_t ZALH(uint8_t ma) { return 0x4000 | ma; }
    static constexpr uint16_t ADDS(uint8_t ma) { return 0x4900 | ma; }

    /** Words in the instruction |inst| starts. */
    static uint_fast8_t words(uint16_t inst);
    /** Whether |inst| reads or writes program memory as data. */
    static bool table(uint16_t inst);
    /** Whether |inst| repeats the instruction after it. */
    static bool repeat(uint16_t inst);
};

}  // namespace tms320c2x
}  // namespace debugger
#endif

// Local Variables:
// mode: c++
// c-basic-offset: 4
// tab-width: 4
// End:
// vim: set ft=cpp et ts=4 sw=4:
