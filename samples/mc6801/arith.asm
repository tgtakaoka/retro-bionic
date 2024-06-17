        cpu     6801
        include "mc6801.inc"

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
;;; @param D v1
;;; @param X v2
;;; @param exprop op
;;; @return D v1
;;; @return X v2
;;; @clobber R0 R1 R2
expr:
        std     vA
        stx     vB
        jsr     print_int16     ; print v1
        bsr     putspace
        ldaa    exprop
        bsr     putchar         ; print op
        bsr     putspace
        ldd     vB
        jsr     print_int16     ; print v2
        ldd     vA
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
        std     vA
        stx     vB
        subd    vB
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
        staa    exprop
        ldd     vA
        ldx     vB
        bsr     expr
        bra     newline

arith:
        ldaa    #'+'
        staa    exprop
        ldd     #18000
        ldx     #28000
        jsr     expr
        addd    vB
        jsr     answer          ; -19536

        ldd     #18000
        ldx     #-18000
        jsr     expr
        addd    vB
        jsr     answer          ; 0

        ldd     #-18000
        ldx     #-18000
        jsr     expr
        addd    vB
        jsr     answer          ; 29536

        ldaa    #'-'
        staa    exprop
        ldd     #-18000
        ldx     #-28000
        jsr     expr
        subd    vB
        jsr     answer          ; -19536

        ldd     #18000
        ldx     #-18000
        jsr     expr
        subd    vB
        jsr     answer          ; 29536

        ldd     #-28000
        ldx     #-18000
        jsr     expr
        subd    vB
        jsr     answer          ; -10000

        ldaa    #'*'
        staa    exprop
        ldd     #100
        ldx     #300
        jsr     expr
        jsr     mul16
        jsr     answer          ; 30000

        ldd     #200
        ldx     #100
        jsr     expr
        jsr     mul16
        jsr     answer          ; 20000

        ldd     #300
        ldx     #-200
        jsr     expr
        jsr     mul16
        jsr     answer          ; 5536

        ldd     #100
        ldx     #-300
        jsr     expr
        jsr     mul16
        jsr     answer          ; -30000

        ldd     #-200
        ldx     #-100
        jsr     expr
        jsr     mul16
        jsr     answer          ; 20000

        ldaa    #'/'
        staa    exprop
        ldd     #30000
        ldx     #100
        jsr     expr
        jsr     div16
        jsr     answer          ; 30

        ldd     #-200
        ldx     #100
        jsr     expr
        jsr     div16
        jsr     answer          ; -2

        ldd     #-30000
        ldx     #-200
        jsr     expr
        jsr     div16
        jsr     answer          ; 150

        ldd     #-30000
        ldx     #78
        jsr     expr
        jsr     div16
        jsr     answer          ; -384

        ldd     #5000
        ldx     #4000
        jsr     comp

        ldd     #5000
        ldx     #5000
        jsr     comp

        ldd     #4000
        ldx     #5000
        jsr     comp

        ldd     #-5000
        ldx     #-4000
        jsr     comp

        ldd     #-5000
        ldx     #-5000
        jsr     comp

        ldd     #-4000
        ldx     #-5000
        jsr     comp

        ldd     #32700
        ldx     #32600
        jsr     comp

        ldd     #32700
        ldx     #32700
        jsr     comp

        ldd     #32600
        ldx     #32700
        jsr     comp

        ldd     #-32700
        ldx     #-32600
        jsr     comp

        ldd     #-32700
        ldx     #-32700
        jsr     comp

        ldd     #-32600
        ldx     #-32700
        jsr     comp

        ldd     #18000
        ldx     #-28000
        jsr     comp

        ldd     #-28000
        ldx     #-28000
        jsr     comp

        ldd     #-28000
        ldx     #18000
        jsr     comp
        rts

        include "arith.inc"
