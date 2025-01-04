;;; -*- mode: asm; mode: flyspell-prog; -*-
        include "im6100.inc"
/// MC6850 Asynchronous Communication Interface Adapter
ACIA    = 07
        include "mc6850.inc"

        *4000
rx_queue_size = 40              / 32 words
rx_queue,
        *.+rx_queue_size
tx_queue_size = 40              / 32 words
tx_queue,
        *.+tx_queue_size

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
I_getchar,      getchar
I_putchar,      putchar

        page    1
initialize,
        cla
        tad     rx_size
        jms     I I_queue_init
        rx_queue
        cla
        tad     tx_size
        jms     I I_queue_init
        tx_queue
        // Initialize ACIA
        cla
        tad     ACIA_config
        iot     ACIA ACIA_control
        tad     RX_INT_TX_NO
        iot     ACIA ACIA_control

loop,
        jms     I I_getchar
        sna                     / Skip if AC != 0
        jmp     halt_to_system
        dca     received_char   / Save character
        tad     received_char
        jms     I I_putchar
        jms     putspace
        jms     put_hex8
        jms     putspace
        jms     put_bin8
        jms     newline
        jmp     loop
halt_to_system,
        hlt

rx_size,        rx_queue_size
tx_size,        tx_queue_size
ACIA_config,    CDS_RESET_gc
RX_INT_TX_NO,  WSB_8N1_gc|RIEB_bm

received_char,  0

char_0,         060             / char '0'
char_x,         170             / char 'x'
put_hex8,       .-.
        cla
        tad     char_0
        jms     I I_putchar     / print '0'
        cla
        tad     char_x
        jms     I I_putchar     / print 'x'
        cla
        tad     received_char
        rtr                     / AC >>= 2
        rtr                     / AC >>= 2
        jms     put_hex4        / print upper 4bit
        cla
        tad     received_char
        jms     put_hex4        / print lower 4bit
        jmp     I put_hex8

lower_nibble,   017             / 0x0F
char_A,         101             / char 'A'
put_hex4,       .-.
        and     lower_nibble
        dca     put_work
        tad     const_10
        cia                     / AC=-10
        tad     put_work
        sma                     / Skip if AC<10
        jmp     put_hex4_hex
put_hex4_dec,
        tad     const_10
        tad     char_0
        skp
put_hex4_hex,
        tad     char_A
        jms     I I_putchar
        jmp     I put_hex4
const_10,      12               / constant 10

put_work,       0
char_b,         142             / char 'b'
const_8,        10              / constant 8
put_count,      0
put_bin8,       .-.
        cla
        tad     char_0
        jms     I I_putchar     / print '0'
        cla
        tad     char_b
        jms     I I_putchar     / print 'b'
        cla
        tad     received_char
        rtl
        rtl
        dca     put_work        / put_work=AC<<4
        tad     const_8
        cia                     / AC=-8
        dca     put_count
put_bin8_loop,
        jms     put_bin1
        isz     put_count       / Skip if print 8 bits
        jmp     put_bin8_loop
        jmp     I put_bin8

put_bin1,       .-.
        cla
        tad     put_work
        ral                     / MSB->L
        dca     put_work        / put_work <<=1
        szl                     / Skip if L=0
        iac                     / AC=1
        tad     char_0          / Add '0'
        jms     I I_putchar
        jmp     I put_bin1

/// Print space
/// @clobber AC
putspace,       .-.
        cla
        tad     char_space
        jms     I I_putchar
        jmp     I putspace
char_space,     040             / char ' '

/// Print new line
/// @clobber AC
newline,        .-.
        cla
        tad     char_CR
        jms     I I_putchar
        cla
        tad     char_LF
        jms     I I_putchar
        jmp     I newline
char_CR,        015             / Carriage return
char_LF,        012             / Line feed

        page

/// Print character
/// @param AC character
putchar,        .-.
        dca     putchar_char    / Save character
        tad     putchar_char
putchar_loop,
        iof
        jms     I I_queue_add
        tx_queue
        ion
        snl                     / Skip if L=1
        jmp     putchar_loop    / tx_queue is full
        cla
        tad     ENABLE_TX_INT   / Enable Tx interrupt
        iot     ACIA ACIA_control
        tad     putchar_char    / Restore character
        jmp     I putchar       / Return
putchar_char,   0
ENABLE_TX_INT,  WSB_8N1_gc|RIEB_bm|TCB_EI_gc

/// Read character
/// @return AC character
getchar,        .-.
getchar_loop,
        iof                     / Disable interrupt
        jms     I I_queue_remove
        rx_queue
        dca     getchar_char    / save character
        ion                     / AC may be broken
        snl cla                 / Skip if L=1
        jmp     getchar_loop    / rx_queue is empty
        tad     getchar_char    / restore character
        jmp     I getchar       / Return
getchar_char,   0

        include "queue.inc"

        page

/// Interrupt service routine
bit_receive,    RDRF_bm
bit_transmit,   TDRE_bm
isr_AC,         0
isr_FLAGS,      0
isr,
        dca     isr_AC
        gtf
        dca     isr_FLAGS
        iot     ACIA ACIA_status
        and     bit_receive
        sna cla                 / Skip if received data exists
        jmp     isr_send
        iot     ACIA ACIA_receive
        jms     I I_queue_add
        rx_queue
isr_send,
        cla
        iot     ACIA ACIA_status
        and     bit_transmit
        sna cla                 / Skip if transmitter is ready
        jmp     isr_exit
        jms     I I_queue_remove
        tx_queue
        snl                     / Skip if L=1
        jmp     isr_send_empty  / L=0 AC=0
        iot     ACIA ACIA_transmit
isr_exit,
        tad     isr_FLAGS
        rtf                     / Restore FLAGS
        cla
        tad     isr_AC          / Restore AC
        jmp     I PC_INTR       / Return
DISABLE_TX_INT,   WSB_8N1_gc|RIEB_bm
isr_send_empty,
        tad     DISABLE_TX_INT
        iot     ACIA ACIA_control
        jmp     isr_exit
