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
        bss     2*16            R0-R15

        org     >2000
stack:          equ     $
rx_queue_size:  equ     128
rx_queue:       bss     rx_queue_size
tx_queue_size:  equ     128
tx_queue:       bss     tx_queue_size

        org     >0200
initialize:
        li      R10, stack
        li      R9, rx_queue
        li      R0, rx_queue_size
        bl      @queue_init
        li      R9, tx_queue
        li      R0, tx_queue_size
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
loop:
        bl      @getchar
        jnc     wait
        soc     R0, R0
        jeq     halt_to_system
echo:
        mov     R0, R1
        bl      @putchar
        li      R0, hibyte(' ')
        bl      @putchar
        bl      @put_hex8
        li      R0, hibyte(' ')
        bl      @putchar
        bl      @put_bin8
        bl      @newline
        jmp     loop
halt_to_system:
        xop     R15,15

        *** Print uint8_t in hex
        *** @param R1 uint8_t value to be printed in hex
        *** @clobber R0
put_hex8:
        dect    R10
        mov     R11, *R10       Push return address
        li      R0, hibyte('0')
        bl      @putchar
        li      R0, hibyte('x')
        bl      @putchar
        mov     R1, R0
        srl     R0, 4
        bl      @put_hex4
        mov     R1, R0
        bl      @put_hex4
        mov     *R10+, R11      Pop return address
        rt
put_hex4:
        andi    R0, hibyte(>0F)
        ci      R0, hibyte(10)
        jl      put_hex4_dec
        ai      R0, hibyte(-10+'A'-'0')
put_hex4_dec:
        ai      R0, hibyte('0')
        jmp     putchar

        *** Print uint8_t in hex
        *** @param R1 uint8_t value to be printed in binary.
        *** @clobber R0
put_bin8:
        dect    R10
        mov     R11, *R10       Push return address
        dect    R10
        mov     R2, *R10        Push R2
        li      R0, hibyte('0')
        bl      @putchar
        li      R0, hibyte('b')
        bl      @putchar
        li      R2, >8000
put_bin8_loop:
        czc     R2, R1
        jeq     put_bin8_zero
        li      R0, hibyte('1')
        jmp     put_bin8_putchar
put_bin8_zero:
        li      R0, hibyte('0')
put_bin8_putchar:
        bl      @putchar
        srl     R2, 1
        ci      R2, >0080
        jne     put_bin8_loop
        mov     *R10+, R2       Pop R2
        mov     *R10+, R11      Pop return address
        rt

        *** Get character
        *** @return R0
        *** @return ST.C 0 if no character
getchar:
        dect    R10
        mov     R11, *R10       Push return address
        dect    R10
        mov     R1, *R10        Push R1
        limi    0               Disable interrupt
        li      R9, rx_queue
        bl      @queue_remove
        limi    15              Enable interrupt
        mov     *R10+, R1       Pop R1
        mov     *R10+, R11      Pop return address
        rt

        *** Newline
        *** @clobber R0
newline:
        dect    R10
        mov     R11, *R10       Push return address
        li      R0, hibyte(>0D)
        bl      @putchar
        li      R0, hibyte(>0A)
        bl      @putchar
        mov     *R10+, R11      Pop return address
        rt

        *** Put character
putchar:
        dect    R10
        mov     R11, *R10       Push return address
        dect    R10
        mov     R1, *R10        Push R1
        limi    0               Disable interrupt
        li      R9, tx_queue
        bl      @queue_add
        limi    15              Enable interrupt
        li      R1, hibyte(TCB_EI_gc)
        socb    R1, @ACIA_control       Enable Tx interrupt
        mov     *R10+, R1       Pop R1
        mov     *R10+, R11      Pop return address
        rt

        include "queue.inc"

int1_isr:
        movb    @ACIA_status, R0
        andi    R0, hibyte(RDRF_bm)
        jeq     int1_isr_send   lo(R0) is cleared
        movb    @ACIA_data, R0
        li      R9, rx_queue
        bl      @queue_add
int1_isr_send:
        movb    @ACIA_status, R0
        andi    R0, hibyte(TDRE_bm)
        jeq     int1_isr_exit
        li      R9, tx_queue
        bl      @queue_remove
        jnc     int1_isr_send_empty
        movb    R0, @ACIA_data
int1_isr_exit:
        rtwp
int1_isr_send_empty:
        * Disable Tx interrupt
        li      R1, hibyte(TCB_EI_gc)
        szcb    R1, @ACIA_control       Disable Tx interrupt
        rtwp
