        cpu     6800
        include "mc6800.inc"

;;; MC6850 Asynchronous Communication Interface Adapter
ACIA:   equ     $DF00
        include "mc6850.inc"

        org     $20
R0:
R0H:    rmb     1
R0L:    rmb     1
R1:
R1H:    rmb     1
R1L:    rmb     1
R2:
R2H:    rmb     1
R2L:    rmb     1
arith_work:
        rmb     2

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
        ldab    ACIA_status
        bitb    #TDRE_bm
        beq     putchar
        staa    ACIA_data
        rts

;;; Print "R1 op R2"
;;; @params A op letter
;;; @clobber R0
expr:
        psha
        ldx     R1
        stx     R0
        jsr     print_int16     ; print R1
        bsr     putspace
        pula
        bsr     putchar         ; print op
        bsr     putspace
        ldx     R2
        stx     R0
        jmp     print_int16     ; print R2

;;; Print " = R0\n"
;;; @clobber R0 R1 R2
answer:
        bsr     putspace
        ldaa    #'='
        bsr     putchar
        bsr     putspace
        jsr     print_int16     ; print R0
        bra     newline

;;; Print "R1 rel R2"
;;; @clobber R0
comp:
        jsr     cmp16
        ldaa    R0L
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
        bsr     expr
        bra     newline

arith:
        ldx     #18000
        stx     R1
        ldx     #28000
        stx     R2
        ldaa    #'+'
        jsr     expr
        jsr     add16           ; R0=R1+R2
        jsr     answer          ; -19536

        ldx     #18000
        stx     R1
        ldx     #-18000
        stx     R2
        ldaa    #'+'
        jsr     expr
        jsr     add16           ; R0=R1+R2
        jsr     answer          ; 0

        ldx     #-18000
        stx     R1
        ldx     #-18000
        stx     R2
        ldaa    #'+'
        jsr     expr
        jsr     add16           ; R0=R1+R2
        jsr     answer          ; 29536

        ldx     #-18000
        stx     R1
        ldx     #-28000
        stx     R2
        ldaa    #'-'
        jsr     expr
        jsr     sub16           ; R0=R1-R2
        jsr     answer          ; -19536

        ldx     #18000
        stx     R1
        ldx     #-18000
        stx     R2
        ldaa    #'-'
        jsr     expr
        jsr     sub16           ; R0=R1-R2
        jsr     answer          ; 29536

        ldx     #-28000
        stx     R1
        ldx     #-18000
        stx     R2
        ldaa    #'-'
        jsr     expr
        jsr     sub16           ; R0=R1-R2
        jsr     answer          ; -10000

        ldx     #100
        stx     R1
        ldx     #300
        stx     R2
        ldaa    #'*'
        jsr     expr
        jsr     mul16           ; R0=R1*R2
        jsr     answer          ; 30000

        ldx     #200
        stx     R1
        ldx     #100
        stx     R2
        ldaa    #'*'
        jsr     expr
        jsr     mul16           ; R0=R1*R2
        jsr     answer          ; 20000

        ldx     #300
        stx     R1
        ldx     #-200
        stx     R2
        ldaa    #'*'
        jsr     expr
        jsr     mul16           ; R0=R1*R2
        jsr     answer          ; 5536

        ldx     #100
        stx     R1
        ldx     #-300
        stx     R2
        ldaa    #'*'
        jsr     expr
        jsr     mul16           ; R0=R1*R2
        jsr     answer          ; -30000

        ldx     #-200
        stx     R1
        ldx     #-100
        stx     R2
        ldaa    #'*'
        jsr     expr
        jsr     mul16           ; R0=R1*R2
        jsr     answer          ; 20000

        ldx     #30000
        stx     R1
        ldx     #100
        stx     R2
        ldaa    #'/'
        jsr     expr
        jsr     div16           ; R0=R1/R2
        jsr     answer          ; 30

        ldx     #-200
        stx     R1
        ldx     #100
        stx     R2
        ldaa    #'/'
        jsr     expr
        jsr     div16           ; R0=R1/R2
        jsr     answer          ; -2

        ldx     #-30000
        stx     R1
        ldx     #-200
        stx     R2
        ldaa    #'/'
        jsr     expr
        jsr     div16           ; R0=R1/R2
        jsr     answer          ; 150

        ldx     #-30000
        stx     R1
        ldx     #78
        stx     R2
        ldaa    #'/'
        jsr     expr
        jsr     div16           ; R0=R1/R2
        jsr     answer          ; -384

        ldx     #5000
        stx     R1
        ldx     #4000
        stx     R2
        jsr     comp

        ldx     #5000
        stx     R1
        ldx     #5000
        stx     R2
        jsr     comp

        ldx     #4000
        stx     R1
        ldx     #5000
        stx     R2
        jsr     comp

        ldx     #-5000
        stx     R1
        ldx     #-4000
        stx     R2
        jsr     comp

        ldx     #-5000
        stx     R1
        ldx     #-5000
        stx     R2
        jsr     comp

        ldx     #-4000
        stx     R1
        ldx     #-5000
        stx     R2
        jsr     comp

        ldx     #32700
        stx     R1
        ldx     #32600
        stx     R2
        jsr     comp

        ldx     #32700
        stx     R1
        ldx     #32700
        stx     R2
        jsr     comp

        ldx     #32600
        stx     R1
        ldx     #32700
        stx     R2
        jsr     comp

        ldx     #-32700
        stx     R1
        ldx     #-32600
        stx     R2
        jsr     comp

        ldx     #-32700
        stx     R1
        ldx     #-32700
        stx     R2
        jsr     comp

        ldx     #-32600
        stx     R1
        ldx     #-32700
        stx     R2
        jsr     comp

        ldx     #18000
        stx     R1
        ldx     #-28000
        stx     R2
        jsr     comp

        ldx     #-28000
        stx     R1
        ldx     #-28000
        stx     R2
        jsr     comp

        ldx     #-28000
        stx     R1
        ldx     #18000
        stx     R2
        jsr     comp
        rts

        include "arith.inc"
