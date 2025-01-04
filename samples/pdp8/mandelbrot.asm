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
I_putspace,     putspace
I_newline,      newline
I_set12,        set12
I_set24,        set24
I_load12,       load12
I_load24,       load24
I_store24,      store24
I_add24,        add24
I_sub24,        sub24
I_neg24,        neg24
I_mul24,        mul24
I_div12,        div12
I_udiv24,       udiv24
I_print_int24,  print_int24
I_debug,        debug
        
arith_ptr,      0       / arithmetic routine work
R0,                     / 24-bit arithmetic accumulator
R0L,            0
R0H,            0
R1,                     / 24-bit arithmetic operand 1
R1L,            0
R1H,            0
R2,                     / 24-bit arithmetic operand 2
R2L,            0
R2H,            0

        page    1
initialize,
        cla
        tad     rx_size
        jms     I I_queue_init
        rx_queue
        tad     tx_size
        jms     I I_queue_init
        tx_queue
        // Initialize ACIA
        tad     ACIA_config
        iot     ACIA ACIA_control
        tad     RX_INT_TX_INT
        iot     ACIA ACIA_control

loop,
        jms     I I_mandelbrot
        jms     I I_newline
        jmp     loop
I_mandelbrot,   mandelbrot

rx_size,        rx_queue_size
tx_size,        tx_queue_size
ACIA_config,    CDS_RESET_gc
RX_INT_TX_INT,  WSB_8N1_gc|RIEB_bm|TCB_EI_gc

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
        iof                     / Disable interrupt
        jms     I I_queue_remove
        rx_queue
        ion                     / Enable interrupt
        jmp     I getchar       / Return

        include "queue.inc"

        page
        include "arith.inc"

        page
        include "mandelbrot.inc"

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
        tad     isr_AC          / Restore AC
        jmp     I PC_INTR       / Return
DISABLE_TX_NO,   WSB_8N1_gc|RIEB_bm
isr_send_empty,
        tad     DISABLE_TX_NO
        iot     ACIA ACIA_control
        jmp     isr_exit

