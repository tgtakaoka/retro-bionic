#ifndef __PINS_IM6100_H__
#define __PINS_IM6100_H__

#define PORT_DX 6     /* GPIO6 */
#define DX_gp 16      /* P6.16-P6.27 */
#define DX_gm 0xFFF   /* P6.16-P6.27 */
#define DX_vp 0       /* DX11-DX0 */
#define PIN_DX11 19   /* P6.16 */
#define PIN_DX10 18   /* P6.17 */
#define PIN_DX9 14    /* P6.18 */
#define PIN_DX8 15    /* P6.19 */
#define PIN_DX7 40    /* P6.20 */
#define PIN_DX6 41    /* P6.21 */
#define PIN_DX5 17    /* P6.22 */
#define PIN_DX4 16    /* P6.23 */
#define PIN_DX3 22    /* P6.24 */
#define PIN_DX2 23    /* P6.25 */
#define PIN_DX1 20    /* P6.26 */
#define PIN_DX0 21    /* P6.27 */
#define PORT_IOC 6    /* GPIO6 */
#define IOC_gp 28     /* P6.28-P6.31 */
#define IOC_gm 0xF    /* P6.28-P6.31 */
#define IOC_vp 0      /* IOC0-IOC3 */
#define PIN_C2 38     /* P6.28 */
#define PIN_C1 39     /* P6.29 */
#define PIN_C0 26     /* P6.30 */
#define PIN_SKP 27    /* P6.31 */
#define PORT_DIR 7    /* GPIO7 */
#define DIR_gp 0      /* P7.00-P7.02 */
#define DIR_gm 0x7    /* P7.00-P7.02 */
#define DIR_vp 0      /* DIR0-DIR2 */
#define PIN_XTB 10    /* P7.00 */
#define PIN_XTC 12    /* P7.01 */
#define PIN_IFETCH 11 /* P7.02 */
#define DIR_XTB 1     /* DIR0 */
#define DIR_XTC 2     /* DIR1 */
#define PIN_XTA 13    /* P7.03 */
#define PIN_LXMAR 8   /* P7.16 */
#define PIN_DATAF 7   /* P7.17 */
#define PIN_INTGNT 36 /* P7.18 */
#define PIN_DMAGNT 37 /* P7.19 */
#define PORT_CNTL 9   /* GPIO9 */
#define CNTL_gp 4     /* P9.04-P9.07 */
#define CNTL_gm 0xF   /* P9.04-P9.07 */
#define CNTL_vp 0     /* CNTL0-CNTL3 */
#define PIN_MEMSEL 2  /* P9.04 */
#define PIN_DEVSEL 3  /* P9.05 */
#define PIN_CPSEL 4   /* P9.06 */
#define PIN_SWSEL 33  /* P6.07 */
#define CNTL_MEMSEL 1 /* CNTL0 */
#define CNTL_DEVSEL 2 /* CNTL1 */
#define CNTL_CPSEL 4  /* CNTL2 */
#define CNTL_SWSEL 8  /* CNTL3 */
#define PIN_LINK 0    /* P6.03 */
#define PIN_RUN 1     /* P6.02 */
#define PIN_OSCOUT 29 /* P9.31 */
#define PIN_CPREQ 6   /* P7.10 */
#define PIN_DMAREQ 32 /* P7.12 */
#define PIN_RESET 28  /* P8.18 */
#define PIN_WAIT 31   /* P8.22 */
#define PIN_RUNHLT 30 /* P8.23 */

#include "pins_pdp8.h"

namespace debugger {
namespace im6100 {

struct PinsIm6100 final : pdp8::PinsPdp8 {
    PinsIm6100();

    void resetPins() override;

private:
    pdp8::Signals *resumeCycle(uint16_t pc = 0) const override;
    pdp8::Signals *prepareCycle() const override;
    pdp8::Signals *completeCycle(pdp8::Signals *signals) const override;
};

}  // namespace im6100
}  // namespace debugger
#endif /* __PINS_IM6100_H__ */

// Local Variables:
// mode: c++
// c-basic-offset: 4
// tab-width: 4
// End:
// vim: set ft=cpp et ts=4 sw=4:
