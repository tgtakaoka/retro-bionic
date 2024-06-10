        *** -*- mode: asm; mode: flyspell-prog; -*-
        include "tms7001.inc"
        *** TMS70C02
        *** Internal Peripherals
IOCNT2: equ     >0101   I/O control register 2
INT3_EDGE:      equ     ?100000         #INT3 1=edge sense, 0=level sense
INT3_POL:       equ     ?010000         #INT3 polarity 1=high/raising, 0=low/falling
INT1_EDGE:      equ     ?000010         #INT1 1=edge sense, 0=level sense
INT1_POL:       equ     ?000001         #INT1 polarity 1=high/raising, 0=low/falling
        *** - SCTL1 has start/stop bit of Timer 3
T3RUN_bm:       equ     ?10000000       1=Start Timer 3
        *** - Serial baud-rate is doubled than TMS7001
        *** - #INT1 and #INT3 can be level active
        *** - No /4 clock option

        *** Local Variables:
        *** comment-start: "*"
        *** End:
