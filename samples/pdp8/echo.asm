;;; -*- mode: asm; mode: flyspell-prog; -*-
        include "im6100.inc"
/// MC6850 Asynchronous Communication Interface Adapter
        include "mc6850.inc"
ACIA    = 07

        page    1
initialize,
        // Initialize ACIA
        cla
        tad     ACIA_config
        iot     ACIA ACIA_control
        tad     RX_NO_TX_NO
        iot     ACIA ACIA_control
        jmp     loop
ACIA_config,    CDS_RESET_gc
RX_NO_TX_NO,    WSB_8N1_gc

loop,
        jms     getchar
        sna                     / Skip if AC != 0
        jmp     halt_to_system
echo,
        jms     putchar
        cia                     / negate AC
        tad     char_CR         / '\r' - AC
        sza                     / Skip if AC == 0
        jmp     loop
        tad     char_LF
        jmp     echo
halt_to_system,
        hlt
char_CR,        015             / Carriage return
char_LF,        012             / Line feed

getchar,        .-.
        cla
getchar_loop,
        iot     ACIA ACIA_status
        and     bit_receive
        sna cla                 / Skip if AC != 0
        jmp     getchar_loop
        iot     ACIA ACIA_receive
        jmp     I getchar
bit_receive,    RDRF_bm

putchar,        .-.
        dca     putchar_char    / Save character
putchar_loop,
        iot     ACIA ACIA_status
        and     bit_transmit
        sna cla                 / Skip if AC != 0
        jmp     putchar_loop
        tad     putchar_char    / Restore character
        iot     ACIA ACIA_transmit
        tad     putchar_char
        jmp     I putchar
putchar_char,   0
bit_transmit,   TDRE_bm

        *ORG_RESET-1
        initialize
        jmp     I .-1

