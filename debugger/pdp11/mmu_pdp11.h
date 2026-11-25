#ifndef __MMU_PDP11_H__
#define __MMU_PDP11_H__

#include <stdint.h>

namespace debugger {
namespace pdp11 {

/**
 * The kernel data space's mapping, as the DCJ11's memory management
 * left it when the debugger took the CPU: where a kernel virtual
 * address the debugger's own code touches lies on the bus.
 */
struct MmuPdp11 {
    static constexpr uint16_t MMR0 = 0177572;
    static constexpr uint16_t MMR3 = 0172516;
    static constexpr uint16_t KIPAR0 = 0172340;
    static constexpr uint16_t KDPAR0 = 0172360;
    static constexpr uint16_t MMR0_ENABLE = 0000001;
    static constexpr uint16_t MMR3_KERNEL_D = 0000004;
    static constexpr uint16_t MMR3_22BIT = 0000020;
    static constexpr uint8_t PAGES = 8;

    uint16_t mmr0 = 0;
    uint16_t mmr3 = 0;
    uint16_t par[PAGES] = {};  // of parBase()'s space

    bool enabled() const { return (mmr0 & MMR0_ENABLE) != 0; }
    // Kernel data goes through the D-space registers only when MMR3
    // enables them.
    uint16_t parBase() const {
        return (mmr3 & MMR3_KERNEL_D) ? KDPAR0 : KIPAR0;
    }
    uint32_t physical(uint16_t va) const {
        if (!enabled())
            return va;
        const uint32_t pa = (uint32_t(par[va >> 13]) << 6) + (va & 017777);
        return pa & ((mmr3 & MMR3_22BIT) ? 017777777 : 0777777);
    }
};

}  // namespace pdp11
}  // namespace debugger
#endif /* __MMU_PDP11_H__ */

// Local Variables:
// mode: c++
// c-basic-offset: 4
// tab-width: 4
// End:
// vim: set ft=cpp et ts=4 sw=4:
