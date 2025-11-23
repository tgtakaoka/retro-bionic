        include "mc68hc11.inc"

;;; MC6850 Asynchronous Communication Interface Adapter
ACIA:   equ     $DF00
        include "../mc6800/mc6850.inc"

        org     $40
R0:
R0H:    rmb     1
R0L:    rmb     1
R1:
R1H:    rmb     1
R1L:    rmb     1
R2:
R2H:    rmb     1
R2L:    rmb     1
sign:   rmb     1

        org     $1000
stack:  equ     *-1             ; MC6800's SP is post-decrement/pre-increment

        org     VEC_SWI
        fdb     VEC_SWI         ; for halt to system

        org     VEC_RESET
        fdb     initialize

        org     $0100
initialize:
        lds     #stack
        ldaa    #CDS_RESET_gc   ; Master reset
        staa    ACIA_control
        ldaa    #WSB_8N1_gc     ; 8 bits + No Parity + 1 Stop Bits
                                ; Transmit, Receive interrupts disabled
        staa    ACIA_control

        jsr     arith
        swi

;;; Print out char
;;; @param A char
;;; @clobber A
putspace:
        ldaa    #' '
        bra     putchar
newline:
        ldaa    #$0D
        bsr     putchar
        ldaa    #$0A
putchar:
        pshb
        ldab    ACIA_status
        bitb    #TDRE_bm
        pulb
        beq     putchar
        staa    ACIA_data
        rts

;;; Print "v1 op v2"
;;; @param X v1
;;; @param Y v2
;;; @param A op
;;; @return D v1
;;; @return X v2
;;; @clobber R1 R2
expr:
        psha
        stx     R1
        sty     R2
        ldd     R1
        jsr     print_int16     ; print v1
        bsr     putspace
        pula
        bsr     putchar         ; print op
        bsr     putspace
        ldd     R2
        jsr     print_int16     ; print v2
        ldd     R1
        ldx     R2
        rts

;;; Print " = v\n"
;;; @param D v
answer:
        psha
        bsr     putspace
        ldaa    #'='
        bsr     putchar
        bsr     putspace
        pula
        jsr     print_int16     ; print v
        bra     newline

;;; Print "v1 rel v2"
;;; @clobber R1 R2
comp:
        stx     R1
        sty     R2
        ldd     R1
        subd    R2
        beq     comp_eq
        blt     comp_lt
        bgt     comp_gt
        ldaa    #'?'
        bra     comp_out
comp_gt:
        ldaa    #'>'
        bra     comp_out
comp_eq:
        ldaa    #'='
        bra     comp_out
comp_lt:
        ldaa    #'<'
comp_out:
        bsr     expr
        bra     newline

arith:
        ldx     #18000
        ldy     #28000
        ldaa    #'+'
        jsr     expr
        addd    R2
        jsr     answer          ; -19536

        ldx     #18000
        ldy     #-18000
        ldaa    #'+'
        jsr     expr
        addd    R2
        jsr     answer          ; 0

        ldx     #-18000
        ldy     #-18000
        ldaa    #'+'
        jsr     expr
        addd    R2
        jsr     answer          ; 29536

        ldx     #-18000
        ldy     #-28000
        ldaa    #'-'
        jsr     expr
        subd    R2
        jsr     answer          ; -19536

        ldx     #18000
        ldy     #-18000
        ldaa    #'-'
        jsr     expr
        subd    R2
        jsr     answer          ; 29536

        ldx     #-28000
        ldy     #-18000
        ldaa    #'-'
        jsr     expr
        subd    R2
        jsr     answer          ; -10000

        ldx     #100
        ldy     #300
        ldaa    #'*'
        jsr     expr
        jsr     mul16
        jsr     answer          ; 30000

        ldx     #200
        ldy     #100
        ldaa    #'*'
        jsr     expr
        jsr     mul16
        jsr     answer          ; 20000

        ldx     #300
        ldy     #-200
        ldaa    #'*'
        jsr     expr
        jsr     mul16
        jsr     answer          ; 5536

        ldx     #100
        ldy     #-300
        ldaa    #'*'
        jsr     expr
        jsr     mul16
        jsr     answer          ; -30000

        ldx     #-200
        ldy     #-100
        ldaa    #'*'
        jsr     expr
        jsr     mul16
        jsr     answer          ; 20000

        ldx     #30000
        ldy     #100
        ldaa    #'/'
        jsr     expr
        jsr     div16           ; R0=R1/R2
        jsr     answer          ; 30

        ldx     #-200
        ldy     #100
        ldaa    #'/'
        jsr     expr
        jsr     div16           ; R0=R1/R2
        jsr     answer          ; -2

        ldx     #-30000
        ldy     #-200
        ldaa    #'/'
        jsr     expr
        jsr     div16           ; R0=R1/R2
        jsr     answer          ; 150

        ldx     #-30000
        ldy     #78
        ldaa    #'/'
        jsr     expr
        jsr     div16           ; R0=R1/R2
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

        include "arith.inc"
