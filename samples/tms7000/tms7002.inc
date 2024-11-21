        *** -*- mode: asm; mode: flyspell-prog; -*-
        *** TMS7002
        include "tms7000.inc"
        include "tms700x_timer1.inc"
        include "tms700x_timer2.inc"
        include "tms700x_serial.inc"
        *** Internal Peripherals
ADDR:   equ     >0105   Port A data direction register
IOCNT1: equ     >0110   I/O control register 1
        *** Serial Port
        *** - SCTL1 has start/stop bit of Timer 3
        *** - Serial baud-rate is doubled than TMS7001
        *** - #INT1 and #INT3 can be level active
        *** - No /4 clock option
SMODE:  equ     >0111   Serial mode (first write)
SCTL0:  equ     >0111   Serial control 0 (write only)
SSTAT:  equ     >0111   Serial status register (read only)
T3DATA: equ     >0114   Timer 3 data
SCTL1:  equ     >0115   Serial control
RXBUF:  equ     >0116   Receive buffer
TXBUF:  equ     >0117   Transmitter buffer
