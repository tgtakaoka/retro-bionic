        include "mc146805e.inc"
        cpu     6805
        option  pc-bits,16

;;; MC6850 Asynchronous Communication Interface Adapter
ACIA:   equ     $17F8
        include "mc6850.inc"

        org     $40
cputype:
        rmb     1
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
        rmb     1
SP:     rmb     1

        org     $0100
stack:  rmb     20

        org     VEC_SWI
        fdb     VEC_SWI         ; for halt to system

        org     VEC_RESET
        fdb     initialize

        org     $1000
initialize:
        include "cputype.inc"
        lda     #CDS_RESET_gc   ; Master reset
        bsr     store_ACIA_control
        lda     #WSB_8N1_gc     ; 8 bits + No Parity + 1 Stop Bits
                                ; Transmit, Receive interrupts disabled
        bsr     store_ACIA_control

        clr     SP
        jsr     arith
        swi                     ; halt to stop

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
        sta     putchar_a
putchar_loop:
        bsr     load_ACIA_status
        bit     #TDRE_bm
        beq     putchar_loop
putchar_data:
        lda     putchar_a
        bsr     store_ACIA_data
        rts
putchar_a:
        rmb     1

;;; Print "R1 op R2"
;;; @params A op letter
;;; @clobber R2 R3 R4
expr:
        sta     expr_op
        ldx     R1H
        lda     R1L
        stx     R0H
        sta     R0L             ; R0=R1
        jsr     print_int16     ; print R1
        bsr     putspace
        lda     expr_op
        bsr     putchar         ; print op
        bsr     putspace
        ldx     R2H
        lda     R2L
        stx     R0H
        sta     R0L             ; R0=R2
        jmp     print_int16     ; print R2
expr_op:
        rmb     1

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
        jsr     cmp16
        lda     R0L
        beq     comp_eq
        bmi     comp_lt
        bpl     comp_gt
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
        ldx     #18000>>8
        lda     #18000&255
        jsr     set_R1
        ldx     #28000>>8
        lda     #28000&255
        jsr     set_R2
        lda     #'+'
        jsr     expr
        jsr     add16           ; R0=R1+R2
        jsr     answer          ; -19536

        ldx     #18000>>8
        lda     #18000&255
        jsr     set_R1
        ldx     #(-18000)>>8
        lda     #(-18000)&255
        jsr     set_R2
        lda     #'+'
        jsr     expr
        jsr     add16           ; R0=R1+R2
        jsr     answer          ; 0

        ldx     #(-18000)>>8
        lda     #(-18000)&255
        jsr     set_R1
        ldx     #(-18000)>>8
        lda     #(-18000)&255
        jsr     set_R2
        lda     #'+'
        jsr     expr
        jsr     add16           ; R0=R1+R2
        jsr     answer          ; 29536

        ldx     #(-18000)>>8
        lda     #(-18000)&255
        jsr     set_R1
        ldx     #(-28000)>>8
        lda     #(-28000)&255
        jsr     set_R2
        lda     #'-'
        jsr     expr
        jsr     sub16           ; R0=R1-R2
        jsr     answer          ; -19536

        ldx     #18000>>8
        lda     #18000&255
        jsr     set_R1
        ldx     #(-18000)>>8
        lda     #(-18000)&255
        jsr     set_R2
        lda     #'-'
        jsr     expr
        jsr     sub16           ; R0=R1-R2
        jsr     answer          ; 29536

        ldx     #(-28000)>>8
        lda     #(-28000)&255
        jsr     set_R1
        ldx     #(-18000)>>8
        lda     #(-18000)&255
        jsr     set_R2
        lda     #'-'
        jsr     expr
        jsr     sub16           ; R0=R1-R2
        jsr     answer          ; -10000

        ldx     #100>>8
        lda     #100&255
        jsr     set_R1
        ldx     #300>>8
        lda     #300&255
        jsr     set_R2
        lda     #'*'
        jsr     expr
        jsr     mul16           ; R0=R1*R2
        jsr     answer          ; 30000

        ldx     #200>>8
        lda     #200&255
        jsr     set_R1
        ldx     #100>>8
        lda     #100&255
        jsr     set_R2
        lda     #'*'
        jsr     expr
        jsr     mul16           ; R0=R1*R2
        jsr     answer          ; 20000

        ldx     #300>>8
        lda     #300&255
        jsr     set_R1
        ldx     #(-200)>>8
        lda     #(-200)&255
        jsr     set_R2
        lda     #'*'
        jsr     expr
        jsr     mul16           ; R0=R1*R2
        jsr     answer          ; 5536

        ldx     #100>>8
        lda     #100&255
        jsr     set_R1
        ldx     #(-300)>>8
        lda     #(-300)&255
        jsr     set_R2
        lda     #'*'
        jsr     expr
        jsr     mul16           ; R0=R1*R2
        jsr     answer          ; -30000

        ldx     #(-200)>>8
        lda     #(-200)&255
        jsr     set_R1
        ldx     #(-100)>>8
        lda     #(-100)&255
        jsr     set_R2
        lda     #'*'
        jsr     expr
        jsr     mul16           ; R0=R1*R2
        jsr     answer          ; 20000

        ldx     #30000>>8
        lda     #30000&255
        jsr     set_R1
        ldx     #100>>8
        lda     #100&255
        jsr     set_R2
        lda     #'/'
        jsr     expr
        jsr     div16           ; R0=R1/R2
        jsr     answer          ; 30

        ldx     #(-200)>>8
        lda     #(-200)&255
        jsr     set_R1
        ldx     #100>>8
        lda     #100&255
        jsr     set_R2
        lda     #'/'
        jsr     expr
        jsr     div16           ; R0=R1/R2
        jsr     answer          ; -2

        ldx     #(-30000)>>8
        lda     #(-30000)&255
        jsr     set_R1
        ldx     #(-200)>>8
        lda     #(-200)&255
        jsr     set_R2
        lda     #'/'
        jsr     expr
        jsr     div16           ; R0=R1/R2
        jsr     answer          ; 150

        ldx     #(-30000)>>8
        lda     #(-30000)&255
        jsr     set_R1
        ldx     #78>>8
        lda     #78&255
        jsr     set_R2
        lda     #'/'
        jsr     expr
        jsr     div16           ; R0=R1/R2
        jsr     answer          ; -384

        ldx     #5000>>8
        lda     #5000&255
        jsr     set_R1
        ldx     #4000>>8
        lda     #4000&255
        jsr     set_R2
        jsr     comp

        ldx     #5000>>8
        lda     #5000&255
        jsr     set_R1
        ldx     #5000>>8
        lda     #5000&255
        jsr     set_R2
        jsr     comp

        ldx     #4000>>8
        lda     #4000&255
        jsr     set_R1
        ldx     #5000>>8
        lda     #5000&255
        jsr     set_R2
        jsr     comp

        ldx     #(-5000)>>8
        lda     #(-5000)&255
        jsr     set_R1
        ldx     #(-4000)>>8
        lda     #(-4000)&255
        jsr     set_R2
        jsr     comp

        ldx     #(-5000)>>8
        lda     #(-5000)&255
        jsr     set_R1
        ldx     #(-5000)>>8
        lda     #(-5000)&255
        jsr     set_R2
        jsr     comp

        ldx     #(-4000)>>8
        lda     #(-4000)&255
        jsr     set_R1
        ldx     #(-5000)>>8
        lda     #(-5000)&255
        jsr     set_R2
        jsr     comp

        ldx     #32700>>8
        lda     #32700&255
        jsr     set_R1
        ldx     #32600>>8
        lda     #32600&255
        jsr     set_R2
        jsr     comp

        ldx     #32700>>8
        lda     #32700&255
        jsr     set_R1
        ldx     #32700>>8
        lda     #32700&255
        jsr     set_R2
        jsr     comp

        ldx     #32600>>8
        lda     #32600&255
        jsr     set_R1
        ldx     #32700>>8
        lda     #32700&255
        jsr     set_R2
        jsr     comp

        ldx     #(-32700)>>8
        lda     #(-32700)&255
        jsr     set_R1
        ldx     #(-32600)>>8
        lda     #(-32600)&255
        jsr     set_R2
        jsr     comp

        ldx     #(-32700)>>8
        lda     #(-32700)&255
        jsr     set_R1
        ldx     #(-32700)>>8
        lda     #(-32700)&255
        jsr     set_R2
        jsr     comp

        ldx     #(-32600)>>8
        lda     #(-32600)&255
        jsr     set_R1
        ldx     #(-32700)>>8
        lda     #(-32700)&255
        jsr     set_R2
        jsr     comp

        ldx     #18000>>8
        lda     #18000&255
        jsr     set_R1
        ldx     #(-28000)>>8
        lda     #(-28000)&255
        jsr     set_R2
        jsr     comp

        ldx     #(-28000)>>8
        lda     #(-28000)&255
        jsr     set_R1
        ldx     #(-28000)>>8
        lda     #(-28000)&255
        jsr     set_R2
        jsr     comp

        ldx     #(-28000)>>8
        lda     #(-28000)&255
        jsr     set_R1
        ldx     #18000>>8
        lda     #18000&255
        jsr     set_R2
        jsr     comp
        rts

        include "arith.inc"

;;; MC68HC05 compatibility
        org     $FFFC
        fdb     $FFFC           ; SWI: halt to system
        fdb     initialize      ; RESET
