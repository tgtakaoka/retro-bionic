#ifndef __INST_TMS3201X_H__
#define __INST_TMS3201X_H__

#include <stdint.h>

namespace debugger {
namespace tms3201x {

struct InstTms3201X {
    static constexpr uint16_t ORG_INT = 0x002;

    /** branch */
    static constexpr uint16_t B = 0xF900;
    /** move P to ACC */
    static constexpr uint16_t PAC = 0x7F8E;

    /** input from IO */
    static constexpr uint16_t IN(uint_fast8_t pa, uint_fast8_t dma) {
        return 0x4000 | (pa << 8) | dma;
    }

    /** output to IO */
    static constexpr uint16_t OUT(uint_fast8_t pa, uint_fast8_t dma) {
        return 0x4800 | (pa << 8) | dma;
    }

    /** store lo16(ACC) */
    static constexpr uint16_t SACL(uint_fast8_t dma) { return 0x5000 | dma; }

    /** store hi16(ACC) */
    static constexpr uint16_t SACH(uint_fast8_t dma) { return 0x5800 | dma; }

    /** load lo16(ACC) */
    static constexpr uint16_t ZALS(uint_fast8_t dma) { return 0x6600 | dma; }

    /** add to hi16(ACC) */
    static constexpr uint16_t ADDH(uint_fast8_t dma) { return 0x6000 | dma; }

    /** store ARn */
    static constexpr uint16_t SAR(uint_fast8_t ar, uint_fast8_t dma) {
        return 0x3000 | (ar << 8) | dma;
    }

    /** load ARP */
    static constexpr uint16_t LARP(uint_fast8_t rp) {
        return 0x6880 | (rp & 1);
    }

    /** load ARn */
    static constexpr uint16_t LAR(uint_fast8_t ar, uint_fast8_t k) {
        return 0x3800 | (ar << 8) | k;
    }

    /** load K to ARn */
    static constexpr uint16_t LARK(uint_fast8_t ar, uint_fast8_t k) {
        return 0x7000 | (ar << 8) | k;
    }

    /** multiply K with T to P (store T) */
    static constexpr uint16_t MPYK(uint_fast16_t k) {
        return 0x8000 | (k & 0x1FFF);
    }

    /** multiply to P (load P) */
    static constexpr uint16_t MPY(uint_fast8_t dma) { return 0x6D00 | dma; }

    /** load T */
    static constexpr uint16_t LT(uint_fast8_t dma) { return 0x6A00 | dma; }

    /** store ST */
    static constexpr uint16_t SST(uint_fast8_t dma) { return 0x7C00 | dma; }

    /** load ST */
    static constexpr uint16_t LST(uint_fast8_t dma) { return 0x7B00 | dma; }

    static uint_fast8_t cycles(uint16_t inst);
};

}  // namespace tms3201x
}  // namespace debugger
#endif

// Local Variables:
// mode: c++
// c-basic-offset: 4
// tab-width: 4
// End:
// vim: set ft=cpp et ts=4 sw=4:
