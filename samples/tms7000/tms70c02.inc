        *** -*- mode: asm; mode: flyspell-prog; -*-
        *** TMS70C02
        include "tms7000.inc"
        include "tms700x_serial.inc"
        *** Internal Peripherals
ADDR:   equ     >0105   Port A data direction register
IOCNT1: equ     >0102   I/O control register 1
IOCNT2: equ     >0101   I/O control register 2
INT3_EDGE:      equ     ?100000         #INT3 1=edge sense, 0=level sense
INT3_POL:       equ     ?010000         #INT3 polarity 1=high/raising, 0=low/falling
INT1_EDGE:      equ     ?000010         #INT1 1=edge sense, 0=level sense
INT1_POL:       equ     ?000001         #INT1 polarity 1=high/raising, 0=low/falling
        *** Timer 1
T1MSDATA:       equ     >010C
T1LSDATA:       equ     >010D
T1CTL1:         equ     >010E
T1_OUT:         equ     ?01000000       1=toggle B1
T1CTL0:         equ     >010F
T1_START:       equ     ?10000000       1=Reload, 0=Stop
T1_SOURCE:      equ     ?01000000       1=External A7, 0=fosc/16
T1_HALT:        equ     ?00100000       1=Halt during idle
        *** TImer 2
T2MSDATA:       equ     >0110
T2LSDATA:       equ     >0111
T2CTL1:         equ     >0112
T2_CASCADE:     equ     ?10000000       1=source is Timer 1
T2_OUT:         equ     ?01000000       1=toggle B0
T2CTL0:         equ     >0113
T2_START:       equ     ?10000000       1=Reload, 0=Stop
T2_SOURCE:      equ     ?01000000       1=External A6, 0=fosc/4
T2_HALT:        equ     ?00100000       1=Halt during idle
        *** Serial Port
        *** - SCTL0 has SCLK enable
        *** - SCTL1 has start/stop bit of Timer 3
        *** - Serial baud-rate is doubled than TMS7001
        *** - #INT1 and #INT3 can be level active
        *** - No /4 clock option
SMODE:  equ     >0114   Serial mode
SCTL0:  equ     >0115   Serial control 0
SSTAT:  equ     >0116   Serial status register (read only)
T3DATA: equ     >0117   Timer 3 data
SCTL1:  equ     >0118   Serial control
RXBUF:  equ     >0119   Receive buffer
TXBUF:  equ     >011A   Transmitter buffer
