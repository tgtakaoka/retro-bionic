        *** -*- mode: asm; mode: flyspell-prog; -*-
        include "tms7000.inc"
        *** TMS7001
        *** Vector
VEC_INT5:       equ     >FFF4   Tuner 2
VEC_INT4:       equ     >FFF6   Serial port
        *** Internal Peripherals
ADDR:   equ     >0105           Port A data direction register
IOCNT1: equ     >0110           I/O control register 1
INT5_F:         equ     ?1000           INT5 flag
INT5_E:         equ     ?0100           INT5 enable
INT4_F:         equ     ?0010           INT4 flag
INT4_E:         equ     ?0001           INT4 enable
SMODE:  equ     >0111           Serial mode (first write)
STOP_bm:        equ     ?10000000       1=2 stop bits. 0=1 stop bits
CMODE_bm:       equ     ?01000000       1=Communication mode, 0=Serial I/O mode
PEVEN_bm:       equ     ?00100000       1=Even parity, 0=Odd parity
PEN_bm:         equ     ?00010000       1=Parity enable, 0=Parity disable
CHAR8_bm:       equ     ?00001100       3=8 bits/char
ASYNC_bm:       equ     ?00000010       1=Asynchronous, 0=Synchronous
MULTI_bm:       equ     ?00000001       1=Intel protocol, 0-Motorola protocol
SCTL0:  equ     >0111           Serial control 0 (write only)
UR_bm:          equ     ?01000000       1=Reset serial port
ER_bm:          equ     ?00010000       1=Reset error flags
RXEN_bm:        equ     ?00000100       1=Receiver enable, 0=A5 is GPIO
SCLKEN_bm:      equ     ?00000010       1=A4 is SCLK, 0=A4 is GPIO
TXEN_bm:        equ     ?00000001       1=Transmitter enable, 0=Transmitter disable
SSTAT:  equ     >0111           Serial status register (read only)
BFKDR_bm:       equ     ?01000000       1=Break detect
FE_bm:          equ     ?00100000       1=Framing error
OE_bm:          equ     ?00010000       1=Overrun error
PE_bm:          equ     ?00001000       1=Parity error
TXE_bm:         equ     ?00000100       1=Transmitter empty
RXRDY_bm:       equ     ?00000010       1=RXBUF ready with new character
TXRDY_bm:       equ     ?00000001       1=TXBYF ready for character
T2DATA: equ     >0112           Timer 2 data
T2CNTL: equ     >0113           Timer 2 control
T3DATA: equ     >0114           Timer 3 data
SCTL1:  equ     >0115           Serial control
CLK_bm:         equ     ?01000000       1=Internal SCLK from Timer3, 0=SCLK
T3ENB_bm:       equ     ?00000100       1=Enable T3 to set INT4 flag
PRE3_bm:        equ     ?00000011       Prescaler reload reg PR
RXBUF:  equ     >0116           Receive buffer
TXBUF:  equ     >0117           Transmitter buffer

        *** Local Variables:
        *** comment-start: "*"
        *** End:
