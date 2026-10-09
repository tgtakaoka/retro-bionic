;;; -*- mode: asm; mode: flyspell-prog; -*-
        cpu     z8002           ; nonsegmented: the Z8002's code
        include "z8001.inc"
        include "../z8002/usart.inc"

stack:  equ     1000H

        org     ORG_RESET
        word    0
        word    FCW_SN          ; system mode, interrupts disabled
        word    0               ; segment 0
        word    init

        org     PSA_VI
        word    0
        word    FCW_SN          ; the handlers run with interrupts disabled
        org     PSA_VI+4+2*RX_ID
        word    0, isr_rx
        org     PSA_VI+4+2*TX_ID
        word    0, isr_tx

        org     0100H
init:
        ld      r15, #stack
        clr     r0
        ldctl   psap, r0        ; the Program Status Area at 0000H
        call    init_io
        ei      vi

receive_loop:
        call    getchar
        jr      nc, receive_loop
        testb   rl0
        jr      z, halt_to_system
echo_back:
        ldb     rl3, rl0
        call    putchar         ; echo
        call    putspace
        call    put_hex8        ; print in hex
        call    putspace
        call    put_bin8        ; print in binary
        call    newline
        jr      receive_loop
halt_to_system:
        sc      #SC_EXIT

;;; Print uint8_t in hex
;;; @param RL3 uint8_t value to be printed in hex.
;;; @clobber RL0
put_hex8:
        ldb     rl0, #'0'
        call    putchar
        ldb     rl0, #'x'
        call    putchar
        ldb     rl0, rl3
        srlb    rl0, #4
        call    put_hex4
        ldb     rl0, rl3
put_hex4:
        andb    rl0, #0FH
        cpb     rl0, #10
        jr      c, put_hex4_digit
        addb    rl0, #'A'-'0'-10
put_hex4_digit:
        addb    rl0, #'0'
        jr      putchar

;;; Print uint8_t in binary
;;; @param RL3 uint8_t value to be printed in binary.
;;; @clobber RL0 R2
put_bin8:
        ldb     rl0, #'0'
        call    putchar
        ldb     rl0, #'b'
        call    putchar
        ldb     rh2, rl3
        ldb     rl2, #8
put_bin8_loop:
        ldb     rl0, #'0'
        rlb     rh2, #1         ; FLAGS.C=MSB
        jr      nc, put_bin8_bit
        incb    rl0, #1
put_bin8_bit:
        call    putchar
        dbjnz   rl2, put_bin8_loop
        ret

        include "../z8002/io.inc"

;;; Return from an interrupt handler. The Z8001 must be segmented to IRET
;;; (its operation in nonsegmented mode is undefined), and then pops the
;;; frame through RR14, the system stack's segment the debugger zeroes
;;; at reset.
isr_return:
        push    @r15, r1
        ldctl   r1, fcw
        set     r1, #15         ; FCW_SEG
        ldctl   fcw, r1
        word    97E1H           ; pop r1, @rr14
        iret
