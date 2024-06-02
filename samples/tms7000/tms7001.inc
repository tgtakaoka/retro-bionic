        *** -*- mode: asm; mode: flyspell-prog; -*-
        include "tms7000.inc"
        *** TMS7001
        *** Vector
VEC_INT5:       equ     >FFF4   Tuner 2
VEC_INT4:       equ     >FFF6   Serial port
        *** Internal Peripherals
ADDR:   equ     >0105           Port A data direction register
IOCNT1: equ     >0110           I/O control register 1
INT5_F: equ     ?1000           INT5 flag
INT5_E: equ     ?0100           INT5 enable
INT4_F: equ     ?0010           INT4 flag
INT4_E: equ     ?0001           INT4 enable
SMODE:  equ     >0111           Serial mode (first write)
SCTL0:  equ     >0111           Serial control 0 (write only)
SSTAT:  equ     >0111           Serial status register (read only)
T2DATA: equ     >0112           Timer 2 data
T2CNTL: equ     >0113           Timer 2 control
T3DATA: equ     >0114           Timer 3 data
SCTL1:  equ     >0115           Serial control
RXBUF:  equ     >0116           Receive buffer
TXBUF:  equ     >0117           Transmitter buffer

        *** Local Variables:
        *** comment-start: "*"
        *** End:
