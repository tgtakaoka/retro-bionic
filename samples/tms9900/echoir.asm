*** -*- mode: asm; mode: flyspell-prog; -*-
        include "tms9900.inc"

        *** MC6850 Asynchronous Communication Interface Adapter
ACIA:   equ     >0080
        include "mc6850.inc"

        org     VEC_RESET
        data    workspace
        data    initialize

        org     VEC_INT1
        data    workspace_int1
        data    int1_isr

        org     VEC_XOP15
        data    VEC_XOP15       for halt to system
        data    VEC_XOP15

hibyte: function v,(v)*256      Hi byte constant

        org     >0100
workspace:
        bss     2*16            R0-R15
workspace_int1:
        bss     2*16            R0~R15

        org     >2000
rx_queue_size:  equ     128
rx_queue:       bss     rx_queue_size

        org     >0200
initialize:
        li      R9, rx_queue
        li      R0, rx_queue_size
        bl      @queue_init
        * Initialize ACIA
        li      R0, hibyte(CDS_RESET_gc)
        movb    R0, @ACIA_control       Master reset
        li      R0, hibyte(WSB_8N1_gc++RIEB_bm)
        movb    R0, @ACIA_control       8 bits + No Parity + 1 Stop Bits
        li      R0, hibyte(VEC_INT1)
        movb    R0, @ACIA_intr          set INT1 name for MC6805 emulator
        jmp     loop

wait:
        idle
loop:   limi    0               Disable interrupt
        li      R9, rx_queue
        bl      @queue_remove
        limi    15              Enable interrupt
        jnc     wait
        soc     R0, R0
        jeq     halt_to_system
echo:   bl      @putchar
        ci      R0, hibyte(>0D) Cariage Return
        jne     loop
        li      R0, hibyte(>0A) Newline
        jmp     echo
halt_to_system:
        xop     R15,15

putchar:
        movb    @ACIA_status, R1
        andi    R1, hibyte(TDRE_bm)
        jeq     putchar
        movb    R0, @ACIA_data
        rt

        include "queue.inc"

int1_isr:
        movb    @ACIA_status, R0
        andi    R0, hibyte(RDRF_bm)
        jeq     int1_isr_exit
        movb    @ACIA_data, R0
        li      R9, rx_queue
        bl      @queue_add
int1_isr_exit:
        rtwp
