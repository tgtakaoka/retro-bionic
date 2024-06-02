        *** -*- mode: asm; mode: flyspell-prog; -*-
        include "tms7001.inc"
        *** TMS7002
        *** Internal Peripherals
IOCNT2: equ     >0101   I/O control register 2
INT3_EDGE:      ?100000 #INT3 1=edge sense, 0=level sense
INT3_POL:       ?010000 #INT3 polarity 1=high/raising, 0=low/falling
INT1_EDGE:      ?000010 #INT1 1=edge sense, 0=level sense
INT1_POL:       ?000001 #INT1 polarity 1=high/raising, 0=low/falling
        *** - SCTL1 has start/stop bit of Timer 3
        *** - Serial baud-rate is doubled than TMS7001
        *** - #INT1 and #INT3 can be level active
        *** - No /4 clock option

        *** Local Variables:
        *** comment-start: "*"
        *** End:
