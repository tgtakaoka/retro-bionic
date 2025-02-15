        *** -*- mode: asm; mode: flyspell-prog; -*-
        *** TMS7001
        include "tms7000.inc"
        include "tms700x_timer1.inc"
        include "tms700x_timer2.inc"
        include "tms700x_serial.inc"
        *** Internal Peripherals
ADDR:   equ     >0105   Port A data direction register
IOCNT1: equ     >0110   I/O control register 1
        *** Timer 2
T2DATA: equ     >0112           Timer 2 data
T2CNTL: equ     >0113           Timer 2 control
        *** Serial Port
SMODE:  equ     >0111   Serial mode (first write)
SCTL0:  equ     >0111   Serial control 0 (write only)
SSTAT:  equ     >0111   Serial status register (read only)
T3DATA: equ     >0114   Timer 3 data
SCTL1:  equ     >0115   Serial control
RXBUF:  equ     >0116   Receive buffer
TXBUF:  equ     >0117   Transmitter buffer
