#ifndef __CHIP_H8500_H__
#define __CHIP_H8500_H__

#include <stdint.h>

namespace debugger {
namespace h8500 {

// What tells the boards apart: the data bus, the address lines, and the
// registers the setup writes.
struct ChipH8500 {
    const char *name;
    // H8/510 in mode 4: 16-bit bus, A0-A23. H8/536 in mode 3: 8-bit bus,
    // A0-A19.
    bool bus16;
    uint32_t addrMask;
    // Wait-state control register: H'F8 selects the pin wait mode with no
    // programmed waits.
    uint16_t wcr;
    // Where #IRQ0's pin function is enabled.
    uint16_t irq0eReg;
    uint8_t irq0eBit;
    // EXTAL's range: the system clock ø is EXTAL/2 and the chips are
    // dynamic.
    uint32_t extalMinHz;
    uint32_t extalMaxHz;
};

// HD6415108 (H8/510 Hardware Manual): WCR H'FF14, IRQCR H'FF1D bit 0 --
// Appendix B; section 5's text has H'FFFD, the external I/O area. ø 0.5-10
// MHz.
constexpr ChipH8500 H8_510{"H8/510", true, 0xFFFFFF, 0xFF14, 0xFF1D, 0,
        1000000, 20000000};
// HD6435368 (H8/534, H8/536 Hardware Manual): WCR H'FF10, SYSCR1 H'FEFC
// bit 5. ø 2-10 MHz suits both the R (0.5-10) and S (2-16) masks.
constexpr ChipH8500 H8_536{"H8/536", false, 0xFFFFF, 0xFF10, 0xFEFC, 5,
        4000000, 20000000};

}  // namespace h8500
}  // namespace debugger
#endif /* __CHIP_H8500_H__ */

// Local Variables:
// mode: c++
// c-basic-offset: 4
// tab-width: 4
// End:
// vim: set ft=cpp et ts=4 sw=4:
