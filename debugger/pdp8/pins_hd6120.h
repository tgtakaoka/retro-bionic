#ifndef __PINS_HD6120_H__
#define __PINS_HD6120_H__

#define PORT_DATA 6    /* GPIO6 */
#define DATA_gp 16     /* P6.16-P6.27 */
#define DATA_gm 0xFFF  /* P6.16-P6.27 */
#define DATA_vp 0      /* DX11-DX0 */
#define PIN_DX11 19    /* P6.16 */
#define PIN_DX10 18    /* P6.17 */
#define PIN_DX9 14     /* P6.18 */
#define PIN_DX8 15     /* P6.19 */
#define PIN_DX7 40     /* P6.20 */
#define PIN_DX6 41     /* P6.21 */
#define PIN_DX5 17     /* P6.22 */
#define PIN_DX4 16     /* P6.23 */
#define PIN_DX3 22     /* P6.24 */
#define PIN_DX2 23     /* P6.25 */
#define PIN_DX1 20     /* P6.26 */
#define PIN_DX0 21     /* P6.27 */
#define PORT_ADDR 6    /* GPIO6 */
#define ADDR_gp 16     /* P6.16-P6.30 */
#define ADDR_gm 0x7FFF /* P6.16-P6.30 */
#define ADDR_vp 0      /* DX11-DX0/EMA2/C1-C0 */
#define PIN_EMA2 38    /* P6.28 */
#define PIN_EMA1 39    /* P6.29 */
#define PIN_EMA0 26    /* P6.30 */
#define PIN_DMAGNT 27  /* P6.31 */
#define PORT_DIR 7     /* GPIO7 */
#define DIR_gp 0       /* P7.00-P7.02 */
#define DIR_gm 0x7     /* P7.00-P7.02 */
#define DIR_vp 0       /* DIR0-DIR3 */
#define PIN_WRITE 10   /* P7.00 */
#define PIN_READ 12    /* P7.01 */
#define PIN_IFETCH 11  /* P7.02 */
#define PIN_IOCLR 13   /* P7.03 */
#define PORT_IOC 7     /* GPIO7 */
#define IOC_gp 16      /* P7.16-P7.19 */
#define IOC_gm 0xE     /* P7.17-P7.19 */
#define IOC_vp 0       /* IOC0-IOC3 */
#define PIN_DATAF 8    /* P7.16 */
#define PIN_C1 7       /* P7.17 */
#define PIN_C0 36      /* P7.18 */
#define PIN_SKIP 37    /* P7.19 */
#define PORT_CNTL 9    /* GPIO9 */
#define CNTL_gp 4      /* P9.04-P9.08 */
#define CNTL_gm 0x1F   /* P9.04-P9.08 */
#define CNTL_vp 0      /* CNTL0-CNTL4 */
#define PIN_LXMAR 2    /* P9.04 */
#define PIN_LXDAR 3    /* P9.05 */
#define PIN_LXPAR 4    /* P9.06 */
#define PIN_MEMSEL 33  /* P9.07 */
#define PIN_INTGNT 5   /* P9.08 */
#define CNTL_LXMAR 1   /* CNTL0 */
#define CNTL_LXDAR 2   /* CNTL1 */
#define CNTL_LXPAR 4   /* CNTL2 */
#define CNTL_MEMSEL 8  /* CNTL3 */
#define CNTL_INTGNT 16 /* CNTL4 */
#define PIN_STRTUP 0   /* P6.03 */
#define PIN_RUN 1      /* P6.02 */
#define PIN_OSCIN 29   /* P9.31 */
#define PIN_CPREQ 6    /* P7.10 */
#define PIN_DMAREQ 32  /* P7.12 */
#define PIN_RESET 28   /* P8.18 */
#define PIN_ACK 31     /* P8.22 */
#define PIN_RUNHLT 30  /* P8.23 */

#include "pins_pdp8.h"

namespace debugger {
namespace hd6120 {

struct PinsHd6120 final : pdp8::PinsPdp8 {
    PinsHd6120();

    void resetPins() override;

private:
    pdp8::Signals *resumeCycle(uint16_t pc = 0) const override;
    pdp8::Signals *prepareCycle() const override;
    pdp8::Signals *completeCycle(pdp8::Signals *signals) const override;
};

}  // namespace hd6120
}  // namespace debugger
#endif /* __PINS_HD6120_H__ */

// Local Variables:
// mode: c++
// c-basic-offset: 4
// tab-width: 4
// End:
// vim: set ft=cpp et ts=4 sw=4:
