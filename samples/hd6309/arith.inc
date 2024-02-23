;;; -*- mode: asm; mode: flyspell-prog; -*-
        cpu     6309

;;; Print signed 16-bit integer as decimal
;;; @param D value
print_int16:
        pshs   D
        tstd
        bpl     print_int16_print
        lda     #'-'
        jsr     putchar
        ldd     ,S
        negd
print_int16_print:
        pshsw
        tfr     D, W
        bsr     print_uint16
        pulsw
        puls    D, PC
;;; Print unsigned 16-bit integer as decimal
;;; @param W value
;;; @clobber D W
print_uint16:
        tstw
        bne     print_uint16_loop
        lda     #'0'
        jmp     putchar
print_uint16_loop:
        tstw
        beq     print_uint16_end ; branch if value == 0
        clrd
        divq    #10             ; D:W/10=W...D
        pshs    B               ; push reminder
        bsr     print_uint16_loop
        puls    A
        ora     #'0'
        jmp     putchar
print_uint16_end:
        rts
