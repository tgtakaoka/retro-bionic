        include "mc68hc08az0.inc"

;;; MC6850 Asynchronous Communication Interface Adapter
ACIA:   equ     $FFE0
        include "../mc6800/mc6850.inc"

        org     RAM_START
R0:
R0H:    rmb     1
R0L:    rmb     1
R1:
R1H:    rmb     1
R1L:    rmb     1
R2:
R2H:    rmb     1
R2L:    rmb     1

        org     VEC_SWI
        fdb     VEC_SWI         ; for halt to system

        org     VEC_RESET
        fdb     initialize

        org     $1000
stack:  equ     *
initialize:
        ldhx    #stack
        txs
        lda     #CDS_RESET_gc   ; Master reset
        sta     ACIA_control
        lda     #WSB_8N1_gc     ; 8 bits + No Parity + 1 Stop Bits
                                ; Transmit, Receive interrupts disabled
        sta     ACIA_control

        jsr     arith
        swi

;;; Print out char
;;; @param A char
;;; @clobber A
putspace:
        lda     #' '
        bra     putchar
newline:
        lda     #$0D
        bsr     putchar
        lda     #$0A
putchar:
        psha
putchar_loop:
        lda     ACIA_status
        bit     #TDRE_bm
        beq     putchar_loop
putchar_data:
        pula
        sta     ACIA_data
        rts

;;; Print "R1 op R2"
;;; @params A op letter
;;; @clobber R2 R3 R4
expr:
        psha
        lda     COP_RESET
        sta     COP_RESET
        ldhx    R1
        sthx    R0
        jsr     print_int16     ; print R1
        bsr     putspace
        pula
        bsr     putchar         ; print op
        bsr     putspace
        ldhx    R2
        sthx    R0
        jsr     print_int16     ; print R2
        rts

;;; Print " = R0\n"
;;; @clobber R0 R1 R2
answer:
        jsr     putspace
        lda     #'='
        jsr     putchar
        jsr     putspace
        jsr     print_int16     ; print R0
        jmp     newline

;;; Print "R1 rel R2"
;;; @clobber R0
comp:
        ldhx    R1
        cphx    R2
        beq     comp_eq
        blt     comp_lt
        bgt     comp_gt
        lda     #'?'
        bra     comp_out
comp_gt:
        lda     #'>'
        bra     comp_out
comp_eq:
        lda     #'='
        bra     comp_out
comp_lt:
        lda     #'<'
comp_out:
        bsr     expr
        jmp     newline

arith:
        ldhx    #18000
        sthx    R1
        ldhx    #28000
        sthx    R2
        lda     #'+'
        jsr     expr
        jsr     add16           ; R0=R1+R2
        jsr     answer          ; -19536

        ldhx    #18000
        sthx    R1
        ldhx    #-18000
        sthx    R2
        lda     #'+'
        jsr     expr
        jsr     add16           ; R0=R1+R2
        jsr     answer          ; 0

        ldhx    #-18000
        sthx    R1
        ldhx    #-18000
        sthx    R2
        lda     #'+'
        jsr     expr
        jsr     add16           ; R0=R1+R2
        jsr     answer          ; 29536

        ldhx    #-18000
        sthx    R1
        ldhx    #-28000
        sthx    R2
        lda     #'-'
        jsr     expr
        jsr     sub16           ; R0=R1-R2
        jsr     answer          ; -19536

        ldhx    #18000
        sthx    R1
        ldhx    #-18000
        sthx    R2
        lda     #'-'
        jsr     expr
        jsr     sub16           ; R0=R1-R2
        jsr     answer          ; 29536

        ldhx    #-28000
        sthx    R1
        ldhx    #-18000
        sthx    R2
        lda     #'-'
        jsr     expr
        jsr     sub16           ; R0=R1-R2
        jsr     answer          ; -10000

        ldhx    #100
        sthx    R1
        ldhx    #300
        sthx    R2
        lda     #'*'
        jsr     expr
        jsr     mul16           ; R0=R1*R2
        jsr     answer          ; 30000

        ldhx    #200
        sthx    R1
        ldhx    #100
        sthx    R2
        lda     #'*'
        jsr     expr
        jsr     mul16           ; R0=R1*R2
        jsr     answer          ; 20000

        ldhx    #300
        sthx    R1
        ldhx    #-200
        sthx    R2
        lda     #'*'
        jsr     expr
        jsr     mul16           ; R0=R1*R2
        jsr     answer          ; 5536

        ldhx    #100
        sthx    R1
        ldhx    #-300
        sthx    R2
        lda     #'*'
        jsr     expr
        jsr     mul16           ; R0=R1*R2
        jsr     answer          ; -30000

        ldhx    #-200
        sthx    R1
        ldhx    #-100
        sthx    R2
        lda     #'*'
        jsr     expr
        jsr     mul16           ; R0=R1*R2
        jsr     answer          ; 20000

        ldhx    #30000
        sthx    R1
        ldhx    #100
        sthx    R2
        lda     #'/'
        jsr     expr
        jsr     div16           ; R0=R1/R2
        jsr     answer          ; 30

        ldhx    #-200
        sthx    R1
        ldhx    #100
        sthx    R2
        lda     #'/'
        jsr     expr
        jsr     div16           ; R0=R1/R2
        jsr     answer          ; -2

        ldhx    #-30000
        sthx    R1
        ldhx    #-200
        sthx    R2
        lda     #'/'
        jsr     expr
        jsr     div16           ; R0=R1/R2
        jsr     answer          ; 150

        ldhx    #-30000
        sthx    R1
        ldhx    #78
        sthx    R2
        lda     #'/'
        jsr     expr
        jsr     div16           ; R0=R1/R2
        jsr     answer          ; -384

        ldhx    #5000
        sthx    R1
        ldhx    #4000
        sthx    R2
        jsr     comp

        ldhx    #5000
        sthx    R1
        ldhx    #5000
        sthx    R2
        jsr     comp

        ldhx    #4000
        sthx    R1
        ldhx    #5000
        sthx    R2
        jsr     comp

        ldhx    #-5000
        sthx    R1
        ldhx    #-4000
        sthx    R2
        jsr     comp

        ldhx    #-5000
        sthx    R1
        ldhx    #-5000
        sthx    R2
        jsr     comp

        ldhx    #-4000
        sthx    R1
        ldhx    #-5000
        sthx    R2
        jsr     comp

        ldhx    #32700
        sthx    R1
        ldhx    #32600
        sthx    R2
        jsr     comp

        ldhx    #32700
        sthx    R1
        ldhx    #32700
        sthx    R2
        jsr     comp

        ldhx    #32600
        sthx    R1
        ldhx    #32700
        sthx    R2
        jsr     comp

        ldhx    #-32700
        sthx    R1
        ldhx    #-32600
        sthx    R2
        jsr     comp

        ldhx    #-32700
        sthx    R1
        ldhx    #-32700
        sthx    R2
        jsr     comp

        ldhx    #-32600
        sthx    R1
        ldhx    #-32700
        sthx    R2
        jsr     comp

        ldhx    #18000
        sthx    R1
        ldhx    #-28000
        sthx    R2
        jsr     comp

        ldhx    #-28000
        sthx    R1
        ldhx    #-28000
        sthx    R2
        jsr     comp

        ldhx    #-28000
        sthx    R1
        ldhx    #18000
        sthx    R2
        jsr     comp
        rts

        include "arith.inc"
