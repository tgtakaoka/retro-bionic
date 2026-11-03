#ifndef __PINS_I8086_BUS_H__
#define __PINS_I8086_BUS_H__

// The address/data bus every 8086-family board wires alike: AD0-AD15 on
// P10-P27 and A16-A19 on P30-P33. The 8088 and V20 carry A8-A15 on
// AD8-AD15's pins.
#define PORT_AD 6    /* GPIO6 */
#define AD_gp 16     /* P6.16-P6.31 */
#define AD_gm 0xFFFF /* P6.16-P6.31 */
#define AD_vp 0      /* AD0-AD15 */
#define PORT_ADL 6   /* GPIO6 */
#define ADL_gp 16    /* P6.16-P6.23 */
#define ADL_gm 0xFF  /* P6.16-P6.23 */
#define ADL_vp 0     /* AD0-AD7 */
#define PIN_AD0 19   /* P6.16 P10 */
#define PIN_AD1 18   /* P6.17 P11 */
#define PIN_AD2 14   /* P6.18 P12 */
#define PIN_AD3 15   /* P6.19 P13 */
#define PIN_AD4 40   /* P6.20 P14 */
#define PIN_AD5 41   /* P6.21 P15 */
#define PIN_AD6 17   /* P6.22 P16 */
#define PIN_AD7 16   /* P6.23 P17 */
#define PIN_AD8 22   /* P6.24 P20 */
#define PIN_AD9 23   /* P6.25 P21 */
#define PIN_AD10 20  /* P6.26 P22 */
#define PIN_AD11 21  /* P6.27 P23 */
#define PIN_AD12 38  /* P6.28 P24 */
#define PIN_AD13 39  /* P6.29 P25 */
#define PIN_AD14 26  /* P6.30 P26 */
#define PIN_AD15 27  /* P6.31 P27 */
// A16-A19 carry S3-S6 from T2 on.
#define PORT_AH 7   /* GPIO7 */
#define AH_gp 0     /* P7.00-P7.03 */
#define AH_gm 0xF   /* P7.00-P7.03 */
#define AH_vp 16    /* A16-A19 */
#define PORT_ST 7   /* GPIO7 */
#define ST_gp 0     /* P7.00-P7.03 */
#define ST_gm 0xF   /* P7.00-P7.03 */
#define ST_vp 0     /* S3-S6 */
#define PIN_A16S 10 /* P7.00 P30 */
#define PIN_A17S 12 /* P7.01 P31 */
#define PIN_A18S 11 /* P7.02 P32 */
#define PIN_A19S 13 /* P7.03 P33 */

#endif /* __PINS_I8086_BUS_H__ */

// Local Variables:
// mode: c++
// c-basic-offset: 4
// tab-width: 4
// End:
// vim: set ft=cpp et ts=4 sw=4:
