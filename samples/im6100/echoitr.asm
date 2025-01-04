;;; -*- mode: asm; mode: flyspell-prog; -*-
        include "im6100.inc"
/// MC6850 Asynchronous Communication Interface Adapter
ACIA    = 07770
        include "mc6850.inc"

        *4000
rx_queue_size = 32
rx_queue,
        *.+rx_queue_size
tx_queue_size = 32
tx_queue,
        *.+tx_queue_size

        *ORG_INTR
        jmp     I isr_addr
isr_addr,
        isr

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
        WSB_8N1_gc|RIEB_bm
rx_size,
        rx_queue_size
tx_size,
        tx_queue_size
initialize,
        cla
        tad     rx_size
        jms     I A_queue_init
        rx_queue
        cla
        tad     tx_size
        jms     I A_queue_init
        txx_queue
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

A_queue_init,   queue_init
A_queue_add,    queue_add
A_queue_remove, queue_remove

getchar,
        0
getchar_loop,
        iof                             / Disable interrupt
        jms     I A_queue_remove
        rx_queue
        ion                             / Enable interrupt
        snl                             / Skip if L=1
        jmp     getchar_loop            / rx_queue is empty
        jmp     I getchar               / Return

putchar,
        0
putchar_loop,
        iof
        jms     I A_queue_add
        tx_queue
        ion
        snl                             / Skip if L=1
        jmp     putchar_loop            / tx_queue is full
        jmp     I putchar               / Return

isr_AC,         0
isr_FLAGS,      0        
isr,
        dca     isr_AC
        gtf
        dca     isr_FLAGS
        tad     I ACIA_status
        and     bit_receive
        sna                             / Skip if received data exists
        jmp     isr_send
        cla
        tad     I ACIA_data             / Received data
        jms     I A_queue_add
        rx_queue
isr_send,
        cla
        tad     I ACIA_status
        and     bit_transmit
        sna                             / Skip if transmitter is ready
        jmp     isr_exit
        jms     A_queue_remove
        tx_queue
        snl                             / Skip if L=1
        jmp     isr_send_empty
        dca     I ACIA_data
isr_exit,
        cla
        tad     isr_FLAGS
        rtf                             / Restore FLAGS
        tad     isr_AC                  / Restore AC
        jmp     I PC_INTR               / Return
bit_receive,
        RDRF_bm
bit_transmit,
        TDRE_bm

        include "queue.inc"
