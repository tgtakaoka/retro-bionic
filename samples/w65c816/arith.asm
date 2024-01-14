;;; -*- mode: asm; mode: flyspell-prog; -*-
        cpu     65816
        .include "w65c816.inc"

;;; MC6850 Asynchronous Communication Interface Adapter
ACIA    =       $DF00
        .include "mc6850.inc"

        *=      VEC_RESET
        .word   initialize

        *=      $1000
stack   =       *-1
initialize:
        clc
        xce                     ; native mode
        rep     #P_X            ; 16-bit index
        longi   on
        ldx     #stack
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
        bra     putchar

putspace:
        lda     #' '
        bra     putchar

;;; Print "X op Y"
;;; @params A op letter
expr:
        pha                     ; save op letter
        jsr     print_int16     ; print X
        jsr     putspace
        pla
        jsr     putchar         ; print op
        jsr     putspace
        phx
        tyx
        jsr     print_int16     ; print Y
        plx
        rts

;;; Print " = X\n"
;;; clobber A
answer:
        jsr     putspace
        lda     #'='
        jsr     putchar
        jsr     putspace
        jsr     print_int16     ; print X
        jmp     newline

;;; Print "X rel Y"
;;; @clobber A
comp:
        jsr     cmpsi2
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
        ldx     #18000
        ldy     #28000
        lda     #'+'
        jsr     expr
        jsr     addsi2
        jsr     answer          ; -19536

        ldx     #18000
        ldy     #-18000
        lda     #'+'
        jsr     expr
        jsr     addsi2
        jsr     answer          ; 0

        ldx     #-18000
        ldy     #-18000
        lda     #'+'
        jsr     expr
        jsr     addsi2
        jsr     answer          ; 29536

        ldx     #-18000
        ldy     #-28000
        lda     #'-'
        jsr     expr
        jsr     subsi2
        jsr     answer          ; -19536

        ldx     #18000
        ldy     #-18000
        lda     #'-'
        jsr     expr
        jsr     subsi2
        jsr     answer          ; 29536

        ldx     #-28000
        ldy     #-18000
        lda     #'-'
        jsr     expr
        jsr     subsi2
        jsr     answer          ; -10000

        ldx     #100
        ldy     #300
        lda     #'*'
        jsr     expr
        jsr     mulsi2
        jsr     answer          ; 30000

        ldx     #200
        ldy     #100
        lda     #'*'
        jsr     expr
        jsr     mulsi2
        jsr     answer          ; 20000

        ldx     #300
        ldy     #-200
        lda     #'*'
        jsr     expr
        jsr     mulsi2
        jsr     answer          ; 5536

        ldx     #100
        ldy     #-300
        lda     #'*'
        jsr     expr
        jsr     mulsi2
        jsr     answer          ; -30000

        ldx     #-200
        ldy     #-100
        lda     #'*'
        jsr     expr
        jsr     mulsi2
        jsr     answer          ; 20000

        ldx     #30000
        ldy     #100
        lda     #'/'
        jsr     expr
        jsr     divsi2
        jsr     answer          ; 300

        ldx     #-200
        ldy     #100
        lda     #'/'
        jsr     expr
        jsr     divsi2
        jsr     answer          ; -2

        ldx     #-30000
        ldy     #-200
        lda     #'/'
        jsr     expr
        jsr     divsi2
        jsr     answer          ; 150

        ldx     #-30000
        ldy     #78
        lda     #'/'
        jsr     expr
        jsr     divsi2
        jsr     answer          ; -384

        ldx     #5000
        ldy     #4000
        jsr     comp

        ldx     #5000
        ldy     #5000
        jsr     comp

        ldx     #4000
        ldy     #5000
        jsr     comp

        ldx     #-5000
        ldy     #-4000
        jsr     comp

        ldx     #-5000
        ldy     #-5000
        jsr     comp

        ldx     #-4000
        ldy     #-5000
        jsr     comp

        ldx     #32700
        ldy     #32600
        jsr     comp

        ldx     #32700
        ldy     #32700
        jsr     comp

        ldx     #32600
        ldy     #32700
        jsr     comp

        ldx     #-32700
        ldy     #-32600
        jsr     comp

        ldx     #-32700
        ldy     #-32700
        jsr     comp

        ldx     #-32600
        ldy     #-32700
        jsr     comp

        ldx     #18000
        ldy     #-28000
        jsr     comp

        ldx     #-28000
        ldy     #-28000
        jsr     comp

        ldx     #-28000
        ldy     #18000
        jsr     comp

        rts

;;; Addition
;;; @param X summand
;;; @param Y addend
;;; @return X summand+addend
;;; @clobber C
        longa   off
addsi2:
        php
        rep     #P_M            ; 16-bit memory
        jsr     add16
        plp
        rts

;;; Subtraction
;;; @param X minuend
;;; @param Y subtrahend
;;; @return X minuend-subtrahend
;;; @clobber C
        longa   off
subsi2:
        php
        rep     #P_M            ; 16-bit memory
        jsr     sub16
        plp
        rts

;;; Signed compare
;;; @param X minuend
;;; @param Y subtrahend
;;; @return C minuend-subtrahend
;;; @return C=0;  BEQ (minuend == subtrahend)
;;;         C=1;  BPL (minuend > subtrahend)
;;;         C=-1; BMI (minuend < subtrahend)
        longa   off
cmpsi2:
        rep     #P_M            ; 16-bit memory
        jsr     cmp16
        sep     #P_M            ; 8-bit memory
        rts

;;; Multiply: result = multiplicand * multiplier
;;; @param X multiplicand
;;; @param Y multiplier
;;; @return X result
;;; @clobber C
        longa   off
mulsi2:
        php
        rep     #P_M            ; 16-bit memory
        jsr     mul16
        plp
        rts

;;; Division: dividend / divisor = quotient ... reminder
;;; @param X dividend
;;; @param Y divisor
;;; @return X quotient
;;; @return Y reminder
;;; @clobber C
        longa   off
divsi2:
        php
        rep     #P_M            ; 16-bit memory
        jsr     div16
        plp
        rts

        .include "arith.inc"
