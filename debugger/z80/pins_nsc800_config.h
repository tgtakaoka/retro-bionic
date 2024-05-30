#ifndef __PINS_NSC800_CONFIG_H__
#define __PINS_NSC800_CONFIG_H__

#define PORT_AD 6      /* GPIO6 */
#define AD_gp 16       /* P6.16-P6.23 */
#define AD_gm 0xFF     /* P6.16-P6.23 */
#define AD_vp 0        /* D0-D7 */
#define PIN_AD0 19     /* P6.16 */
#define PIN_AD1 18     /* P6.17 */
#define PIN_AD2 14     /* P6.18 */
#define PIN_AD3 15     /* P6.19 */
#define PIN_AD4 40     /* P6.20 */
#define PIN_AD5 41     /* P6.21 */
#define PIN_AD6 17     /* P6.22 */
#define PIN_AD7 16     /* P6.23 */
#define PORT_ADDR 6    /* GPIO6 */
#define ADDR_gp 16     /* P6.16-P6.31 */
#define ADDR_gm 0xFFFF /* P6.16-P6.31 */
#define ADDR_vp 0      /* A0-A7 */
#define PIN_ADDR8 22   /* P6.24 */
#define PIN_ADDR9 23   /* P6.25 */
#define PIN_ADDR10 20  /* P6.26 */
#define PIN_ADDR11 21  /* P6.27 */
#define PIN_ADDR12 38  /* P6.28 */
#define PIN_ADDR13 39  /* P6.29 */
#define PIN_ADDR14 26  /* P6.30 */
#define PIN_ADDR15 27  /* P6.31 */
#define PIN_CLK 10     /* P7.00 */
#define PIN_IOM 12     /* P7.01 */
#define PIN_ALE 11     /* P7.02 */
#define PIN_RSTC 7     /* P7.17 */
#define PIN_RSTB 36    /* P7.18 */
#define PIN_RSTA 37    /* P7.19 */
#define PORT_CNTL 9    /* GPIO9 */
#define CNTL_gp 4      /* P9.04-P9.08 */
#define CNTL_gm 0x1F   /* P9.04-P9.08 */
#define CNTL_vp 0      /* CNTL0-CNTL4 */
#define PIN_RD 2       /* P9.04 */
#define PIN_WR 3       /* P9.05 */
#define PIN_S0 4       /* P9.06 */
#define PIN_S1 33      /* P9.07 */
#define PIN_INTA 5     /* P9.08 */
#define CNTL_RD 0x01   /* CNTL0 */
#define CNTL_WR 0x02   /* CNTL1 */
#define CNTL_S 0x0C    /* CNTL2-CNTL3 */
#define CNTL_INTA 0x10 /* CNTL4 */
#define CNTL_IOM 0x20  /* CNTL5 */
#define PIN_PS 0       /* P6.03 */
#define PIN_RFSH 1     /* P6.02 */
#define PIN_XIN 29     /* P9.31 */
#define PIN_RESETIN 28 /* P8.18 */
#define PIN_BREQ 31    /* P8.22 */
#define PIN_BACK 30    /* P8.23 */

#endif /* __PINS_NSC800_CONFIG_H__ */

// Local Variables:
// mode: c++
// c-basic-offset: 4
// tab-width: 4
// End:
// vim: set ft=cpp et ts=4 sw=4:
