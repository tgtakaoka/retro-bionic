;;; -*- mode: asm; mode: flyspell-prog; -*-
        include "im6100.inc"
/// MC6850 Asynchronous Communication Interface Adapter
ACIA    = 07770
        include "mc6850.inc"

        *7776
        initialize
        *7777
        jmp     I 7776

        *200
ACIA_control,
ACIA_status,
        ACIA_control_a
ACIA_data,
        ACIA_data_a
ACIA_config,
        CDS_RESET_gc
        WSB_8N1_gc
initialize,
        // Initialize ACIA
        cla
        tad     ACIA_config
        dca     I ACIA_control
        tad     ACIA_config+1
        dca     I ACIA_control

loop,
        jms     getchar
        sna                     / Skip if AC != 0
        jmp     halt_to_system
echo,
        jms     putchar
        tad     char_CR         / AC - '\r'
        sza                     / Skip if AC == 0
        jmp     loop
        tad     char_LF
        jmp     echo
char_CR,
        -015                    / Carriage return
char_LF,
        012                     / Line feed
halt_to_system,
        hlt

getchar,
        0
getchar_loop,
        cla
        tad     I ACIA_status
        and     bit_receive
        sna                             / Skip if AC != 0
        jmp     getchar_loop
        cla
        tad     I ACIA_data
        jmp     I getchar
bit_receive,
        RDRF_bm

putchar,
        0
        dca     putchar_char            / Save character
putchar_loop,
        cla
        tad     I ACIA_status
        and     bit_transmit
        sna                             / Skip if AC != 0
        jmp     putchar_loop
        cla
        tad     putchar_char            / Restore character
        dca     I ACIA_data
        tad     putchar_char
        jmp     I putchar
putchar_char,
        0
bit_transmit,
        TDRE_bm
