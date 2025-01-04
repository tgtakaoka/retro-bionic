;;; -*- mode: asm; mode: flyspell-prog; -*-
        include "im6100.inc"
/// MC6850 Asynchronous Communication Interface Adapter
ACIA    = 07
        include "mc6850.inc"

        *4000
rx_queue_size = 40              / 32 words
rx_queue,
        *.+rx_queue_size

        *ORG_INTR
        jmp     I .+1
        isr

        *ORG_RESET-1
        initialize
        jmp     I .-1

        *PAGE0
I_queue_init,   queue_init
I_queue_add,    queue_add
I_queue_remove, queue_remove

        page    1
initialize,
        cla
        tad     queue_size
        jms     I I_queue_init
        rx_queue
        // Initialize ACIA
        cla
        tad     ACIA_config
        iot     ACIA ACIA_control
        tad     RX_INT_TX_NO
        iot     ACIA ACIA_control
        jmp     loop
ACIA_config,    CDS_RESET_gc
RX_INT_TX_NO,   WSB_8N1_gc|RIEB_bm
queue_size,     rx_queue_size

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
getchar_loop,
        iof                     / Disable interrupt
        jms     I I_queue_remove
        rx_queue
        dca     getchar_char    / save character
        ion                     / AC may be broken
        snl cla                 / Skip if L=1
        jmp     getchar_loop
        tad     getchar_char    / restore character
        jmp     I getchar       / Return
getchar_char,   0

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

        include "queue.inc"

bit_receive,    RDRF_bm
isr_AC,         0
isr_FLAGS,      0
isr,
        dca     isr_AC
        gtf
        dca     isr_FLAGS
        iot     ACIA ACIA_status
        and     bit_receive
        sna cla                 / Skip if received data exists
        jmp     isr_exit
        iot     ACIA ACIA_receive
        jms     I I_queue_add
        rx_queue
isr_exit,
        cla
        tad     isr_FLAGS
        rtf                     / Restore FLAGS
        cla
        tad     isr_AC          / Restore AC
        jmp     I PC_INTR       / Return
