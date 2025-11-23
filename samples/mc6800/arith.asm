        include "mc6800.inc"

;;; MC6850 Asynchronous Communication Interface Adapter
ACIA:   equ     $DF00
        include "mc6850.inc"

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
exprop: rmb     1
vA:     rmb     2
vB:     rmb     2

        org     $1000
stack:  equ     *-1             ; MC6800's SP is post-decrement/pre-increment

        org     $FFF6           ; MC68HC11 SWI
        fdb     $FFF6

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

;;; Print "v1 op R2"
;;; @param D v1
;;; @param X v2
;;; @param exprop op
;;; @return D v1
;;; @return X v2
;;; @clobber R0 R1 R2
expr:
        pshb
        psha
        stx     vB
        jsr     print_int16     ; print v1
        bsr     putspace
        ldaa    exprop
        bsr     putchar         ; print op
        bsr     putspace
        ldaa    vB
        ldab    vB+1
        jsr     print_int16     ; print R2
        pula
        pulb
        ldx     vB
        rts

;;; Print " = v\n"
;;; @param D v
;;; @clobber R0 R1 R2
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
;;; @param D v1
;;; @param X v2
;;; @clobber R0 R1 R2
comp:
        staa    vA
        stab    vA+1
        stx     vB
        stx     R2
        ldx     vA
        stx     R1
        jsr     cmp16
        beq     comp_eq
        bmi     comp_lt
        bpl     comp_gt
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
        staa    exprop
        ldaa    vA
        ldab    vA+1
        ldx     vB
        bsr     expr
        bra     newline

arith:
        ldaa    #'+'
        staa    exprop
        ldaa    #(18000) >> 8
        ldab    #(18000) & $FF
        ldx     #28000
        jsr     expr
        addb    vB+1
        adca    vB
        jsr     answer          ; -19536

        ldaa    #(18000) >> 8
        ldab    #(18000) & $FF
        ldx     #-18000
        jsr     expr
        addb    vB+1
        adca    vB
        jsr     answer          ; 0

        ldaa    #(-18000) >> 8
        ldab    #(-18000) & $FF
        ldx     #-18000
        jsr     expr
        addb    vB+1
        adca    vB
        jsr     answer          ; 29536

        ldaa    #'-'
        staa    exprop
        ldaa    #(-18000) >> 8
        ldab    #(-18000) & $FF
        ldx     #-28000
        jsr     expr
        subb    vB+1
        sbca    vB
        jsr     answer          ; -19536

        ldaa    #(18000) >> 8
        ldab    #(18000) & $FF
        ldx     #-18000
        jsr     expr
        subb    vB+1
        sbca    vB
        jsr     answer          ; 29536

        ldaa    #(-28000) >> 8
        ldab    #(-28000) & $FF
        ldx     #-18000
        jsr     expr
        subb    vB+1
        sbca    vB
        jsr     answer          ; -10000

        ldaa    #'*'
        staa    exprop
        ldaa    #(100) >> 8
        ldab    #(100) & $FF
        ldx     #300
        jsr     expr
        jsr     mul16
        jsr     answer          ; 30000

        ldaa    #(200) >> 8
        ldab    #(200) & $FF
        ldx     #100
        jsr     expr
        jsr     mul16
        jsr     answer          ; 20000

        ldaa    #(300) >> 8
        ldab    #(300) & $FF
        ldx     #-200
        jsr     expr
        jsr     mul16
        jsr     answer          ; 5536

        ldaa    #(100) >> 8
        ldab    #(100) & $FF
        ldx     #-300
        jsr     expr
        jsr     mul16
        jsr     answer          ; -30000

        ldaa    #(-200) >> 8
        ldab    #(-200) & $FF
        ldx     #-100
        jsr     expr
        jsr     mul16
        jsr     answer          ; 20000

        ldaa    #'/'
        staa    exprop
        ldaa    #(30000) >> 8
        ldab    #(30000) & $FF
        ldx     #100
        jsr     expr
        jsr     div16
        jsr     answer          ; 30

        ldaa    #(-200) >> 8
        ldab    #(-200) & $FF
        ldx     #100
        jsr     expr
        jsr     div16
        jsr     answer          ; -2

        ldaa    #(-30000) >> 8
        ldab    #(-30000) & $FF
        ldx     #-200
        jsr     expr
        jsr     div16
        jsr     answer          ; 150

        ldaa    #(-30000) >> 8
        ldab    #(-30000) & $FF
        ldx     #78
        jsr     expr
        jsr     div16
        jsr     answer          ; -384

        ldaa    #(5000) >> 8
        ldab    #(5000) & $FF
        ldx     #4000
        jsr     comp

        ldaa    #(5000) >> 8
        ldab    #(5000) & $FF
        ldx     #5000
        jsr     comp

        ldaa    #(4000) >> 8
        ldab    #(4000) & $FF
        ldx     #5000
        jsr     comp

        ldaa    #(-5000) >> 8
        ldab    #(-5000) & $FF
        ldx     #-4000
        jsr     comp

        ldaa    #(-5000) >> 8
        ldab    #(-5000) & $FF
        ldx     #-5000
        jsr     comp

        ldaa    #(-4000) >> 8
        ldab    #(-4000) & $FF
        ldx     #-5000
        jsr     comp

        ldaa    #(32700) >> 8
        ldab    #(32700) & $FF
        ldx     #32600
        jsr     comp

        ldaa    #(32700) >> 8
        ldab    #(32700) & $FF
        ldx     #32700
        jsr     comp

        ldaa    #(32600) >> 8
        ldab    #(32600) & $FF
        ldx     #32700
        jsr     comp

        ldaa    #(-32700) >> 8
        ldab    #(-32700) & $FF
        ldx     #-32600
        jsr     comp

        ldaa    #(-32700) >> 8
        ldab    #(-32700) & $FF
        ldx     #-32700
        jsr     comp

        ldaa    #(-32600) >> 8
        ldab    #(-32600) & $FF
        ldx     #-32700
        jsr     comp

        ldaa    #(18000) >> 8
        ldab    #(18000) & $FF
        ldx     #-28000
        jsr     comp

        ldaa    #(-28000) >> 8
        ldab    #(-28000) & $FF
        ldx     #-28000
        jsr     comp

        ldaa    #(-28000) >> 8
        ldab    #(-28000) & $FF
        ldx     #18000
        jsr     comp
        rts

        include "arith.inc"
