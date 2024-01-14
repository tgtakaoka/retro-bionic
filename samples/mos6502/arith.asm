;;; -*- mode: asm; mode: flyspell-prog; -*-
        cpu     6502
        .include "mos6502.inc"

;;; MC6850 Asynchronous Communication Interface Adapter
ACIA    =       $DF00
        .include "mc6850.inc"

        *=      VEC_RESET
        .word   initialize

;;; Work area for arith.inc
        *=      $10
R0:
R0L:    .byte   0
R0H:    .byte   0
R1:
R1L:    .byte   0
R1H:    .byte   0
R2:
R2L:    .byte   0
R2H:    .byte   0
arith_work:
        .word   0

        *=      $1000
stack   =       $01ff
initialize:
        ldx     #stack & 0xFF
        txs
        cld                     ; clear decimal flag
        lda     #CDS_RESET_gc   ; Master reset
        sta     ACIA_control
        lda     #WSB_8N1_gc     ; 8 bits + No Parity + 1 Stop Bits
                                ; Transmit, Receive interrupts disabled
        sta     ACIA_control

        jsr     arith
        brk
        .byte   0               ; halt to system

;;; Print out char
;;; @param A char
;;; @clobber A
putchar:
        pha                     ; save A
putchar_loop:
        lda     ACIA_status
        and     #TDRE_bm
        beq     putchar_loop
        pla                     ; restore A
        sta     ACIA_data
        rts

newline:
        lda     #$0D
        jsr     putchar
        lda     #$0A
        bne     putchar         ; always branch

putspace:
        lda     #' '
        bne     putchar         ; always branch

;;; Print "R1 op R2"
;;; @params A op letter
;;; @clobber R0
expr:
        pha                     ; save op letter
        lda     R1H
        ldx     R1L
        sta     R0H
        stx     R0L
        jsr     print_int16
        jsr     putspace
        pla                     ; restore op letter
        jsr     putchar
        jsr     putspace
        lda     R2H
        ldx     R2L
        sta     R0H
        stx     R0L
        jmp     print_int16

;;; Print " = R0\n"
answer:
        jsr     putspace
        lda     #'='
        jsr     putchar
        jsr     putspace
        jsr     print_int16
        jmp     newline

;;; Print "R1 rel R2"
;;; @clobber R0 R1 R2
comp:
        jsr     cmpsi2
        lda     R0L
        beq     comp_eq
        bmi     comp_lt
        bpl     comp_gt
        lda     #'?'
        bne     comp_out        ; always branch
comp_gt:
        lda     #'>'
        bne     comp_out
comp_eq:
        lda     #'='
        bne     comp_out
comp_lt:
        lda     #'<'
comp_out:
        jsr     expr
        jmp     newline

arith:
        jsr     set_R1
        .word   18000
        jsr     set_R2
        .word   28000
        lda     #'+'
        jsr     expr
        jsr     addsi2
        jsr     answer          ; -19536

        jsr     set_R1
        .word   18000
        jsr     set_R2
        .word   -18000
        lda     #'+'
        jsr     expr
        jsr     addsi2
        jsr     answer          ; 0

        jsr     set_R1
        .word   -18000
        jsr     set_R2
        .word   -18000
        lda     #'+'
        jsr     expr
        jsr     addsi2
        jsr     answer          ; 29536

        jsr     set_R1
        .word   -18000
        jsr     set_R2
        .word   -28000
        lda     #'-'
        jsr     expr
        jsr     subsi2
        jsr     answer          ; -19536

        jsr     set_R1
        .word   18000
        jsr     set_R2
        .word   -18000
        lda     #'-'
        jsr     expr
        jsr     subsi2
        jsr     answer          ; 29536

        jsr     set_R1
        .word   -28000
        jsr     set_R2
        .word   -18000
        lda     #'-'
        jsr     expr
        jsr     subsi2
        jsr     answer          ; -10000

        jsr     set_R1
        .word   100
        jsr     set_R2
        .word   300
        lda     #'*'
        jsr     expr
        jsr     mulsi2
        jsr     answer          ; 30000

        jsr     set_R1
        .word   200
        jsr     set_R2
        .word   100
        lda     #'*'
        jsr     expr
        jsr     mulsi2
        jsr     answer          ; 20000

        jsr     set_R1
        .word   300
        jsr     set_R2
        .word   -200
        lda     #'*'
        jsr     expr
        jsr     mulsi2
        jsr     answer          ; 5536

        jsr     set_R1
        .word   100
        jsr     set_R2
        .word   -300
        lda     #'*'
        jsr     expr
        jsr     mulsi2
        jsr     answer          ; -30000

        jsr     set_R1
        .word   -200
        jsr     set_R2
        .word   -100
        lda     #'*'
        jsr     expr
        jsr     mulsi2
        jsr     answer          ; 20000

        jsr     set_R1
        .word   30000
        jsr     set_R2
        .word   100
        lda     #'/'
        jsr     expr
        jsr     divsi2
        jsr     answer          ; 300

        jsr     set_R1
        .word   -200
        jsr     set_R2
        .word   100
        lda     #'/'
        jsr     expr
        jsr     divsi2
        jsr     answer          ; -2

        jsr     set_R1
        .word   -30000
        jsr     set_R2
        .word   -200
        lda     #'/'
        jsr     expr
        jsr     divsi2
        jsr     answer          ; 150

        jsr     set_R1
        .word   -30000
        jsr     set_R2
        .word   78
        lda     #'/'
        jsr     expr
        jsr     divsi2
        jsr     answer          ; -384

        jsr     set_R1
        .word   5000
        jsr     set_R2
        .word   4000
        jsr     comp

        jsr     set_R1
        .word   5000
        jsr     set_R2
        .word   5000
        jsr     comp

        jsr     set_R1
        .word   4000
        jsr     set_R2
        .word   5000
        jsr     comp

        jsr     set_R1
        .word   -5000
        jsr     set_R2
        .word   -4000
        jsr     comp

        jsr     set_R1
        .word   -5000
        jsr     set_R2
        .word   -5000
        jsr     comp

        jsr     set_R1
        .word   -4000
        jsr     set_R2
        .word   -5000
        jsr     comp

        jsr     set_R1
        .word   32700
        jsr     set_R2
        .word   32600
        jsr     comp

        jsr     set_R1
        .word   32700
        jsr     set_R2
        .word   32700
        jsr     comp

        jsr     set_R1
        .word   32600
        jsr     set_R2
        .word   32700
        jsr     comp

        jsr     set_R1
        .word   -32700
        jsr     set_R2
        .word   -32600
        jsr     comp

        jsr     set_R1
        .word   -32700
        jsr     set_R2
        .word   -32700
        jsr     comp

        jsr     set_R1
        .word   -32600
        jsr     set_R2
        .word   -32700
        jsr     comp

        jsr     set_R1
        .word   18000
        jsr     set_R2
        .word   -28000
        jsr     comp

        jsr     set_R1
        .word   -28000
        jsr     set_R2
        .word   -28000
        jsr     comp

        jsr     set_R1
        .word   -28000
        jsr     set_R2
        .word   18000
        jsr     comp

        rts

        .include "arith.inc"
